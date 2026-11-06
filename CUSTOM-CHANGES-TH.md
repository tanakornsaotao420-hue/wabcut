# WabCut Speech Review 0.6.1 — คู่มือและรายการแก้ไข

ซอร์สสำหรับ fork tanakornsaotao420-hue/wabcut สาขา speech-review
ตัวติดตั้งชุดนี้ต้องผ่าน workflow Windows full build ก่อนใช้งาน; อย่าสับสนกับ build #4 ซึ่งมีเฉพาะการแก้ context overflow และฟีเจอร์ก่อนหน้า

## สิ่งที่รวมในรุ่นนี้

- ขอบคลิป/แทร็กและพื้นหลัง Effect Controls เป็นเหลี่ยม เปลี่ยนกลับได้ใน Settings > Appearance
- คลื่นเสียงสีฟ้าแสดง source channels จริง; ขยายความสูงแทร็กเพื่อเห็น L/R ชัดเจน
- Speech review แบ่งคำขอตามขนาด UTF-8 ของ JSON และคำสั่งระบบ แทนการนับคำอย่างเดียว รองรับ local context 8192 โดยเผื่อ output และ chat template
- หน้าถอดเสียงเลือก large-v3-turbo (เริ่มต้น), base หรือ tiny ได้
- โมเดล Whisper และ VAD ที่ยังไม่มีดาวน์โหลดเมื่อกดถอดเสียง ใช้ระบบเครือข่ายและตรวจไฟล์ของแอป ไม่ต้องเปิด PowerShell
- เลือกภาษา Auto / Thai / English และใส่คำศัพท์เฉพาะได้ จำโมเดล/ภาษาครั้งถัดไป แต่ไม่บันทึกคำศัพท์เฉพาะไว้ทั่วทุกโปรเจกต์
- Transcribe again เปิดการตั้งค่าใหม่และ Replace transcript ถอดจริงแทนการคืน cache เก่า ผลที่ถอดสำเร็จจะแทนข้อความเดิม; ไฟล์ที่ถอดไม่สำเร็จยังมีข้อความเดิม
- Clear transcript ล้างข้อความที่แสดงใน session นี้ โดยไม่ตัดวิดีโอ เสียง หรือซับ ปุ่ม Clear เดิมสำหรับยกเลิกเลือกคำเปลี่ยนชื่อเป็น Deselect
- ไม่ถอดทั้ง sequence หากเลือก Selected clips โดยไม่มีคลิปที่เลือก
- แสดงข้อผิดพลาดเริ่มงานในหน้า Transcript และเพิ่มทางเข้า AI settings จาก Clean up
- ล้างข้อความยืนยันโมเดลเก่าเมื่อเปลี่ยนโมเดล Assistant เพื่อไม่ให้เลือก Local แล้วยังค้างข้อความว่าใช้ Gemini

## เริ่มใช้งานฟรีสำหรับภาษาไทย

1. นำเข้าวิดีโอ วางใน timeline แล้วเปิด Transcript > Sequence
2. เลือก On this computer เลือกโมเดล large-v3-turbo และ Language = Thai
3. ใส่ชื่อที่ถอดผิดบ่อยใน Vocabulary hints เช่น `Asana, Demeter, workflow, use case, HR, Operations` คำเหล่านี้เป็นคำใบ้ ไม่ใช่คำสั่งให้เติมข้อความ
4. เปิด track ที่มีเสียงพูด เช่น A1 แล้วกด Transcribe sequence
5. ครั้งแรกต้องต่ออินเทอร์เน็ตเพื่อดาวน์โหลด Turbo ประมาณ 574 MB และ VAD ประมาณ 1 MB เสียงไม่ถูกอัปโหลดสำหรับการถอดแบบ Local หลังมีโมเดลแล้วใช้ offline ได้
6. หากมี transcript เดิม ให้กด Transcribe again… ตั้งโมเดล/ภาษา แล้วกด Replace transcript ไม่ต้องลบคลิปและไม่ต้องใช้ Delete
7. ถอดเสร็จให้ฟังตรวจชื่อเฉพาะและตัวเลข การเลือกโมเดลใหญ่ขึ้นไม่รับประกันว่าตรง 100% ใช้ Fix… แก้คำที่เลือก หรือ Correct from Script เมื่อมีบทที่ตรงกับเสียง

โมเดลเก็บแยกจากโฟลเดอร์วิดีโอ เปลี่ยนวิดีโอไม่ต้องดาวน์โหลดโมเดลใหม่ หากย้ายไฟล์ที่โปรเจกต์เดิมอ้างอิง ต้องเชื่อมไฟล์ที่หายกับตำแหน่งใหม่ใน Library ก่อน

## ล้างข้อความอย่างปลอดภัย

- Clear transcript… > Clear text: ซ่อน transcript ของเสียงใน sequence นี้ตลอด session ปัจจุบัน ไม่แก้ timeline หรือ captions
- Transcript ใช้ร่วมกันในระดับไฟล์เสียง คลิปอื่นที่ใช้ไฟล์/stream เดียวกันอาจถูกล้างการแสดงด้วย
- เมื่อเปิดโปรแกรมใหม่ cache เดิมอาจกลับมา หากต้องการแทนที่ถาวรให้ใช้ Transcribe again… > Replace transcript
- Delete / Remove ในการแก้ไขด้วยข้อความเป็นการตัดคลิปจริง ไม่ใช่ล้าง transcript
- ซับที่สร้างไปแล้วไม่ได้เปลี่ยนเองจากการถอดใหม่ ให้ตรวจและอัปเดต captions จาก transcript ตามต้องการ

## วิเคราะห์ช่วงคำพูดที่ควรตรวจ

1. Settings > AI > Models เลือก Assistant model
2. แบบฟรี Local: ไป Local assistant กด Download ของโมเดลที่เลือก แล้ว Start หยุดเล่นวิดีโอก่อน หากขึ้น paused for playback or export
3. แบบ API: ไป Providers and keys วาง key ของผู้ให้บริการ กด Save and check จากนั้นเลือกโมเดลใน Models หากกรอก model ID เองต้องเป็น ID ที่ผู้ให้บริการรองรับ แล้วกด Use การเก็บ key สำเร็จยังไม่ยืนยันว่า model ID เรียกใช้ได้
4. ไป Transcript > Clean up ใส่บทอ้างอิงสั้น ๆ หากมี แล้วกด Analyze speech with AI… ตรวจข้อมูล/ค่าใช้จ่ายในหน้าขอยืนยัน
5. ระบบส่งข้อความถอดเสียงและบท ไม่ส่งเสียงหรือวิดีโอสำหรับ speech review ส่วน cloud transcription เป็นอีกงานหนึ่งที่ต้องส่งเสียงและขอยืนยันแยก
6. ฟังตรง timecode ของแต่ละคำแนะนำก่อนเลือกตัด ระบบไม่เลือกคำแนะนำ AI เพื่อลบอัตโนมัติ
7. Find ใช้ค้นช่วงเงียบ/คำแทรกในเครื่องได้โดยไม่ต้องมี chat model
8. Preview > Remove เมื่อตรวจแล้ว และ Undo หากต้องการย้อนกลับ

## ข้อจำกัดที่ต้องทราบ

- AI review อ่าน transcript ไม่ได้ฟังเสียง จึงตรวจการออกเสียงหรือเสียงสะดุดที่ ASR ละทิ้งไม่ได้
- context 8192 ใช้งานได้ด้วยการแบ่งคำขอเล็กลง หากบทอ้างอิงหรือ ASR entry เดียวใหญ่เกินงบ โปรแกรมจะแจ้งให้ย่อก่อนส่ง ไม่ตัดข้อความทิ้งเงียบ ๆ
- ทั้งงานยังจำกัด 6000 หน่วยคำและบท UTF-8 24 KB; งบคำขอจริงเล็กกว่านี้ บทยาวอาจต้องย่อให้สั้นลง
- โมโนแสดงช่องเดียว nested/multicam mix หรือแทร็กเตี้ยอาจแสดงรวม; waveform เป็นเสียงต้นทางก่อน effects/routing
- ปิด optional C2PA SDK ใน preset Windows เนื่องจาก upstream archive ดาวน์โหลดล้มเหลว รุ่นนี้ไม่ลงลายเซ็น Content Credentials แบบฝังไฟล์ แต่ยังมี provenance sidecar

## การทดสอบและสร้างตัวติดตั้ง

Windows build #4 (commit 77997cc) คอมไพล์ ทดสอบ และแพ็กผ่านแล้ว โดยไม่รวม GPU/audio-device tests ชุดแก้ไข 0.6.1 หลังจากนั้นต้องตรวจใน build ใหม่ ไม่มีการอ้างว่าผ่าน live API หรือทดลองติดตั้งบนเครื่องผู้ใช้แล้ว

เพิ่มกรณีทดสอบ: ล้าง/ถอดใหม่ไม่เปลี่ยน timeline, งานที่ล้มเหลวเก็บข้อความเดิม, จำภาษา/โมเดล, ปฏิเสธโมเดลไม่รู้จัก และไม่ถอดคลิปทั้งหมดเมื่อ selection ว่าง

ใช้ GitHub Actions > Windows full build > Run workflow > speech-review หรือ PowerShell จาก source:

```powershell
.\tools\build-speech-review-windows.ps1
```

หลัง workflow สำเร็จ ดาวน์โหลด artifact `wabcut-windows-x64` แตก ZIP แล้วเลือก MSI เพื่อติดตั้งใหม่ ปิด WabCut และสำรอง project ก่อนอัปเดต

คง LICENSE และ THIRD-PARTY-NOTICES ของ upstream
