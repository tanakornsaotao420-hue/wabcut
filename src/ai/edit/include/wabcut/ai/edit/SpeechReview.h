#pragma once
#include <wabcut/ai/Chat.h>
#include <wabcut/ai/edit/Review.h>

namespace wabcut::ai::edit {
inline constexpr std::size_t kSpeechReviewChunk = 300;
inline constexpr std::size_t kSpeechReviewOverlap = 12;
// Conservative UTF-8 byte budget, including JSON and system instructions.
// Reserve 4096 output tokens and 1024 chat-template tokens within an 8192 context.
inline constexpr std::size_t kSpeechReviewInputBytes = 3072;
std::size_t speechReviewChunkSize(const SequenceTranscript& transcript, std::string_view script,
                                  std::size_t first);
// Suggestions reference word IDs in this exact transcript snapshot, never generated timestamps.
ChatRequest speechReviewRequest(const SequenceTranscript& transcript, std::string_view script,
                                std::size_t first, std::size_t count);
std::expected<std::vector<ReviewItem>, Error> parseSpeechReview(
    const SequenceTranscript& transcript, std::string_view response,
    std::size_t first, std::size_t count, bool hasScript);
} // namespace wabcut::ai::edit
