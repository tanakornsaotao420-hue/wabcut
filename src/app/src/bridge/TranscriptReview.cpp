#include "TranscriptBridge.h"
#include "AiController.h"
#include <wabcut/ai/edit/SpeechReview.h>
#include <set>
#include <tuple>
#include <stdexcept>

#include <wabcut/ai/AudioEnergy.h>
#include <wabcut/ai/edit/MediaServices.h>

#include <algorithm>

namespace wabcut::app {
namespace {
// review() calls prefetch once with all candidate ranges. Decoding and filesystem access stay on this worker;
// individual rmsDb calls only read the resulting memory cache. Cooperative cancellation reaches the decoder.
class ReviewEnergy final : public ai::edit::AudioEnergyProbe {
public:
    ReviewEnergy(std::filesystem::path project, std::stop_token stop, std::function<void(float)> progress)
        : m_project(std::move(project)), m_stop(stop), m_progress(std::move(progress)) {}
    void prefetch(std::span<const Range> ranges) const override {
        std::map<std::pair<std::string, int>, std::vector<TimeRange>> groups;
        for (const auto& r : ranges) {
            if (m_stop.stop_requested())
                return;
            if (!r.media)
                continue;
            if (const auto path = ai::edit::mediaFilePath(*r.media, m_project))
                groups[{*path, ai::edit::audioContainerStream(*r.media, r.audioStream)}].push_back(r.source);
        }
        std::size_t done = 0;
        for (const auto& [key, spans] : groups) {
            if (m_stop.stop_requested())
                return;
            const auto result = m_service.run(
                ai::EnergyRequest{key.first, key.second, spans, false}, m_stop, [&](float fraction) {
                    m_progress((static_cast<float>(done) + fraction) / static_cast<float>(groups.size()));
                });
            (void)result; // an unavailable range remains unmeasured; review retains its uncertainty
            ++done;
        }
    }
    std::optional<double> rmsDb(const MediaItem& media, int stream, TimeRange range) const override {
        if (m_stop.stop_requested())
            return {};
        const auto path = ai::edit::mediaFilePath(media, m_project);
        if (!path)
            return {};
        const auto known = m_service.cached(*path, ai::edit::audioContainerStream(media, stream), range);
        return known ? std::optional<double>(known->rmsDb) : std::nullopt;
    }

private:
    std::filesystem::path m_project;
    std::stop_token m_stop;
    std::function<void(float)> m_progress;
    mutable ai::AudioEnergyService m_service;
};
} // namespace

void TranscriptBridge::cancelReview() {
    ++m_reviewGeneration;
    if (m_controller)
        m_controller->setTimelineHighlights(QStringLiteral("speechReview"), {});
    for (auto& worker : m_reviewWorkers)
        worker.thread.request_stop();
    // Joining only finished workers never waits for decoding on the GUI thread.
    std::erase_if(m_reviewWorkers, [](const ReviewWorker& worker) { return worker.finished->load(); });
    if (m_reviewBusy) {
        m_reviewBusy = false;
        m_reviewProgress = 0;
        emit reviewProgressChanged();
    }
}

void TranscriptBridge::setReviewScript(const QString& script) {
    if (script == m_reviewScript) return;
    cancelReview();
    cancelPreview();
    m_review.clear();
    m_reviewRemove.clear();
    m_reviewScript = script;
    emit reviewOptionsChanged();
    emit reviewChanged();
}

QVariantMap TranscriptBridge::speechReviewConsent() {
    if (!m_ai || m_transcript.empty()) {
        setMessage(tr("Transcribe the sequence and configure a chat model in AI settings first"));
        return {};
    }
    if (m_transcript.size() > 6000 || m_reviewScript.toUtf8().size() > 24000) {
        setMessage(tr("Review up to 6000 words and a script up to 24 KB at a time; split a longer sequence first"));
        return {};
    }
    const auto models = m_ai->platform().resolve(ai::Capability::Chat);
    if (!models || models->empty()) {
        setMessage(models ? tr("No chat model configured") : QString::fromStdString(models.error().message));
        return {};
    }
    const QString id = QString::fromStdString(models->front().model->id);
    auto prompt = m_ai->consentPrompt(id, QStringLiteral("chat"),
        tr("Transcript text (%1 words) and the reference script; no audio or video. Suggestions require listening.")
            .arg(m_transcript.size()));
    prompt[QStringLiteral("modelId")] = id;
    ai::MicroUsd estimate = 0;
    bool priced = true;
    const auto script = m_reviewScript.trimmed().toStdString();
    for (std::size_t first = 0; first < m_transcript.size();) {
        const auto count = ai::edit::speechReviewChunkSize(m_transcript, script, first);
        if (count == 0) {
            setMessage(tr("Reference script or a transcript entry is too long for local speech review. Shorten the reference script or split the long transcript entry first."));
            return {};
        }
        const auto request = ai::edit::speechReviewRequest(m_transcript, script, first, count);
        const auto flight = m_ai->platform().preflightChat(id.toStdString(), request);
        if (!flight) {
            setMessage(QString::fromStdString(flight.error().message));
            return {};
        }
        estimate += flight->cost.ceiling;
        priced = priced && flight->cost.priced;
        if (first + count == m_transcript.size()) break;
        first += count - std::min(ai::edit::kSpeechReviewOverlap, count / 2);
    }
    prompt[QStringLiteral("cost")] = priced
        ? tr("Estimated ceiling: USD %1 (actual usage may differ)").arg(static_cast<double>(estimate) / 1000000.0, 0, 'f', 4)
        : tr("Price unavailable for this model; check your provider's pricing");
    return prompt;
}

QString TranscriptBridge::analyzeSpeech(const QString& modelId, bool allowOnce) {
    if (!m_ai || !m_controller || m_transcript.empty() || modelId.isEmpty()) {
        setMessage(tr("Transcribe the sequence and select an available AI chat model first"));
        return m_message;
    }
    if (m_transcript.size() > 6000 || m_reviewScript.toUtf8().size() > 24000) {
        setMessage(tr("Review up to 6000 words and a script up to 24 KB at a time"));
        return m_message;
    }
    auto chat = m_ai->platform().chat(modelId.toStdString(), ai::CallOptions{allowOnce, {}});
    if (!chat) {
        setMessage(QString::fromStdString(chat.error().message));
        return m_message;
    }
    startReview(std::shared_ptr<ai::IChatModel>(std::move(*chat)));
    return {};
}

void TranscriptBridge::seekReview(int index) {
    if (!m_controller || index < 0 || static_cast<std::size_t>(index) >= m_review.size()) return;
    const auto& item = m_review[static_cast<std::size_t>(index)];
    m_controller->setPlayhead(std::max(Ticks{}, item.range.start - Ticks{kTicksPerSecond}));
    TimelineHighlight mark;
    mark.range = item.range;
    mark.tracks.push_back(m_transcript[item.firstWord].track);
    m_controller->setTimelineHighlights(QStringLiteral("speechReview"), {mark});
}

void TranscriptBridge::refreshReview() { startReview(); }

void TranscriptBridge::startReview(std::shared_ptr<ai::IChatModel> chat) {
    cancelReview();
    cancelPreview();
    m_review.clear();
    m_reviewRemove.clear();
    setMessage({});
    emit reviewChanged();
    if (!m_controller || !m_mapped || m_transcript.empty())
        return;
    ai::edit::ReviewOptions options;
    options.fillers = m_reviewFillers;
    options.silenceThreshold =
        m_silenceSeconds > 0
            ? std::optional<Ticks>(Ticks{static_cast<std::int64_t>(m_silenceSeconds * kTicksPerSecond)})
            : std::nullopt;
    const auto generation = m_reviewGeneration;
    const auto snapshot = m_mapped;
    const auto transcript = m_transcript;
    const auto project = m_controller->projectFile();
    const auto selection = m_controller->selection();
    const auto sequence = m_controller->currentSequenceId();
    const auto runner = m_reviewRunner;
    const auto script = m_reviewScript.trimmed().toStdString();
    const QPointer<TranscriptBridge> self(this);
    m_reviewBusy = true;
    m_reviewProgress = 0;
    emit reviewProgressChanged();
    auto finished = std::make_shared<std::atomic<bool>>(false);
    std::jthread thread([this, self, generation, snapshot, transcript, project, selection, sequence, options,
                         runner, finished, chat, script](std::stop_token stop) {
        const auto progress = [this, self, generation](float value) {
            QMetaObject::invokeMethod(
                this,
                [self, generation, value] {
                    if (!self || generation != self->m_reviewGeneration)
                        return;
                    self->m_reviewProgress = std::clamp(static_cast<double>(value), 0.0, 1.0);
                    emit self->reviewProgressChanged();
                },
                Qt::QueuedConnection);
        };
        std::vector<ai::edit::ReviewItem> result;
        QString error;
        try {
            if (!stop.stop_requested()) {
                const auto localProgress = [&](float value) { progress(chat ? value * 0.25f : value); };
                if (runner)
                    result = runner(transcript, *snapshot, options, stop, localProgress);
                else {
                    ReviewEnergy probe(project, stop, localProgress);
                    result = ai::edit::review(transcript, *snapshot, options, &probe);
                }
                if (chat) {
                    std::vector<ai::edit::ReviewItem> suggestions;
                    std::set<std::tuple<ai::edit::ReviewKind, std::size_t, std::size_t>> seen;
                    for (std::size_t first = 0; first < transcript.size() && !stop.stop_requested();) {
                        const auto count = ai::edit::speechReviewChunkSize(transcript, script, first);
                        if (count == 0)
                            throw std::runtime_error("Reference script or a transcript entry is too long for local speech review; shorten it before retrying");
                        const auto request = ai::edit::speechReviewRequest(transcript, script, first, count);
                        const auto response = chat->chat(request, {}, stop);
                        if (!response) throw std::runtime_error(response.error().message);
                        if (response->stopReason != ai::StopReason::EndTurn)
                            throw std::runtime_error("Incomplete AI response; no AI suggestions were applied");
                        auto parsed = ai::edit::parseSpeechReview(transcript, response->message.text(), first, count, !script.empty());
                        if (!parsed) throw std::runtime_error(parsed.error().message);
                        for (auto& item : *parsed)
                            if (seen.emplace(item.kind, item.firstWord, item.lastWord).second)
                                suggestions.push_back(std::move(item));
                        progress(0.25f + 0.75f * static_cast<float>(first + count) / static_cast<float>(transcript.size()));
                        if (first + count == transcript.size()) break;
                        first += count - std::min(ai::edit::kSpeechReviewOverlap, count / 2);
                    }
                    result.insert(result.end(), suggestions.begin(), suggestions.end());
                    std::stable_sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
                        return a.range.start < b.range.start;
                    });
                }
            }
        } catch (const std::exception& e) {
            error = QString::fromUtf8(e.what());
        }
        const bool cancelled = stop.stop_requested();
        QMetaObject::invokeMethod(
            this,
            [self, generation, snapshot, selection, sequence, result = std::move(result), error, cancelled] {
                if (!self || generation != self->m_reviewGeneration || !self->m_controller)
                    return;
                self->m_reviewBusy = false;
                self->m_reviewProgress = cancelled ? 0 : 1;
                emit self->reviewProgressChanged();
                if (cancelled || self->m_mapped != snapshot || self->m_controller->snapshot() != snapshot ||
                    self->m_controller->selection() != selection ||
                    self->m_controller->currentSequenceId() != sequence)
                    return;
                if (!error.isEmpty()) {
                    self->setMessage(error);
                    return;
                }
                self->m_review = result;
                std::vector<TimelineHighlight> highlights;
                for (const auto& item : result) {
                    const bool local = item.kind == ai::edit::ReviewKind::Filler || item.kind == ai::edit::ReviewKind::Silence;
                    self->m_reviewRemove.push_back(local && item.confidence >= 0.5f);
                    TimelineHighlight mark;
                    mark.range = item.range;
                    mark.tracks.push_back(self->m_transcript[item.firstWord].track);
                    highlights.push_back(std::move(mark));
                }
                self->m_controller->setTimelineHighlights(QStringLiteral("speechReview"), std::move(highlights));
                emit self->reviewChanged();
            },
            Qt::QueuedConnection);
        finished->store(true);
    });
    m_reviewWorkers.push_back({std::move(finished), std::move(thread)});
}
} // namespace wabcut::app
