# WabCut Speech Review — source checkpoint 0.1

สถานะ: ซอร์สโค้ดสำหรับพัฒนาต่อ ยังไม่ใช่ EXE/ตัวติดตั้งที่ผ่านการทดสอบ Windows

## สิ่งที่แก้
- ขอบคลิปและแทร็กเป็นเหลี่ยม รวมพื้นหลัง Effect Controls; เปลี่ยนกลับเป็นขอบมนได้ใน Settings > Appearance
- สัญลักษณ์คีย์เฟรมยังบอก interpolation ตามเดิม
- คลื่นเสียงฟ้าบนพื้นน้ำเงินเข้ม แยก source channels จริง มีป้าย L/R เมื่อพื้นที่พอ และเพิ่มความสูงแทร็กเสียง
- โมโนช่องเดียว; nested/multicam mix และแทร็กที่เตี้ยมากยังแสดงรวม
- คลื่นเสียงแสดง source channels ไม่ใช่เสียงหลัง channel routing/effects
- Clean up เพิ่ม Analyze speech with AI และช่องบทอ้างอิง
- API วิเคราะห์ข้อความถอดเสียง: คำซ้ำ เริ่มพูดใหม่ ข้อความน่าสงสัย และคำต่างจากบท
- ใช้ chat model/API key จาก Settings > AI เดิม; แสดงสิ่งที่จะส่งและประมาณราคาก่อนเรียก
- ตรวจช่วงเงียบและคำแทรกด้วยระบบเดิมในเครื่อง
- AI ส่งกลับรหัสคำ โปรแกรมผูกกับเวลาใน snapshot เอง ไม่มีการใช้เวลาที่ AI เดา
- รายการ AI ไม่ถูกเลือกเพื่อลบอัตโนมัติ; กด timecode ไปก่อนจุดนั้น 1 วินาที แล้วเล่นฟัง
- เลือกจุดตัด > Preview > Remove; ใช้ระบบ undo เดิม
- ยกเลิกผลที่ล้าสมัยเมื่อแก้ timeline หรือเปลี่ยน sequence

## ข้อจำกัด
- การวิเคราะห์ใหม่อ่าน transcript ไม่ได้ฟังเสียง จึงตรวจเสียงสะดุดที่ ASR ละทิ้งไม่ได้
- ไม่มีบทจะไม่ยืนยันว่าพูดผิด; ความต่างจากบทอาจเป็นการเรียบเรียงที่ถูกต้อง
- จำกัด 6000 คำและบท UTF-8 24 KB ต่อครั้ง; แบ่งคำเป็นช่วงละ 300 ทับซ้อน 12 คำ
- API response ผิด schema/เกินช่วง/ข้ามผู้พูดจะถูกปฏิเสธ ต้องวิเคราะห์ใหม่
- ยังไม่ทดสอบ live API, Qt UI, Windows EXE, การ export จริง

## การตรวจสอบที่ทำแล้วในรอบก่อน checkpoint
- ชุดทดสอบ review เดิมและใหม่: 649 assertions ใน 6 test cases ผ่าน (focused harness, actual model/AI source)
- SpeechReview.cpp และ Review.cpp คอมไพล์ผ่านด้วย GCC 13/C++23
- ตัวอ่าน waveform ผ่านการตรวจ stereo/mono/รวมช่อง/ช่วงและ channel ไม่ถูกต้องแบบแยกส่วน
- การ build core เต็มเริ่มได้แต่ยังไม่เสร็จ; ไม่ได้อ้างว่าชุดทดสอบทั้งหมดผ่าน
- รวมกับ upstream dcb21f0 แล้ว; ต้องตรวจ full build อีกครั้ง

## Build Windows
อ่าน docs/building.md เพื่อติดตั้ง Visual Studio, Qt และ vcpkg รุ่นที่โปรเจกต์กำหนดก่อน
จาก PowerShell ที่โฟลเดอร์ wabcut:

```powershell
.\tools\build-speech-review-windows.ps1
```

สคริปต์ configure, build, test แล้วจึงสร้าง MSI/portable ZIP ใน build/win-msvc/package
อีกทางหนึ่งใช้ workflow .github/workflows/windows-full.yml ใน repository ของคุณบน GitHub Actions
ต้องมีการเชื่อมต่อบัญชีและนำซอร์สชุดนี้เข้า repository ก่อน จึงสั่ง build ทางนั้นได้

## วิธีใช้เมื่อ build สำเร็จ
1. Settings > AI > Providers: ตั้ง API key และเลือก chat model
2. นำเข้าวิดีโอและถอดเสียงใน Transcript
3. Clean up: วางบท (ถ้ามี), กด Analyze speech with AI และตรวจค่าใช้จ่าย
4. กด timecode ของรายการแล้วเล่นฟัง เลือกจุดที่ต้องการตัดเอง
5. Preview > Remove; Undo เพื่อย้อนกลับ

ยังคง LICENSE และ THIRD-PARTY-NOTICES ของ upstream
