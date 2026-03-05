# UDS_3 (SecurityAccess 0x27) 퀴즈

> UDS_3_Analysis.md를 읽으며 학습한 내용을 체크하세요.
> 먼저 **문제 1~5번**을 모두 풀고, 답은 뒤에서 확인하세요!

---

## 📝 문제 1: SecurityAccess의 필요성

**OTA 플로우에서 SecurityAccess(0x27)가 필요한 이유는?**

다음 중 가장 적절한 답을 고르세요:

A) CPU 속도를 높이기 위해  
B) 누구나 펌웨어를 수정할 수 있는 게 아니라, **인증된 진단기만** 할 수 있도록 제한하기 위해  
C) 메모리를 절약하기 위해  
D) ECUReset 후에 꼭 필요합니다!

---

## 📝 문제 2: Seed/Key 2단계 메커니즘

**다음 CAN 로그를 분석하세요:**

```
[TX] 7D3: 02 27 01 55 55 55 55 55    ← A단계
[RX] 7DB: 04 67 01 12 34 AA AA AA    ← B단계
[TX] 7D3: 04 27 02 56 78 55 55 55    ← C단계
[RX] 7DB: 02 67 02 AA AA AA AA AA    ← D단계
```

**질문:**

**(2-1) A단계에서 0x27은 무엇이고, 0x01은 무엇인가?**

**(2-2) B단계에서 ECU가 응답하는 0x12, 0x34는 뭘까?**

**(2-3) C단계에서 진단기가 0x56, 0x78을 보내기 전에 한 일은?**

다음 중 가장 적절한 답:
- A) B단계의 0x1234를 그냥 복사해서 보냄
- B) Seed(0x1234)와 고정 암호를 XOR해서 Key 생성
- C) 0x1234를 2배로 계산
- D) 어디서 받은 값을 그냥 하드코딩된 Key(0x5678)로 보냄

**(2-4) D단계에서 응답이 긍정(0x67)이라는 건 뭘 의미하나?**

---

## 📝 문제 3: 세션 제약

**강의록 P.6의 "Table 23"을 보면:**

- DiagSessionControl(0x10): Default O, Extended O
- ECUReset(0x11): Default O, Extended O
- **SecurityAccess(0x27): Default ✗, Extended ✓**

**질문: SecurityAccess가 Default 세션에서 불가능한 이유는?**

다음 중 가장 적절한 답을 고르세요:

A) Default 세션은 느리니까 인증이 못 함  
B) 펌웨어 수정은 Extended(확장) 세션에서만 가능한데, 사전에 인증을 못 하는 이유가 없으니까 제약  
C) Table 23에서 그렇게 규정했으니까  
D) 하드웨어가 Default에서는 암호를 못 처리함  

---

## 📝 문제 4: Seed와 Key 이해

**코드를 보면:**

```c
uint16 seed = 0x1234;  // ECU에서 생성
key = ((uint16)stBswDcmData.diagDataBufferRx[2] << 8u) 
    | stBswDcmData.diagDataBufferRx[3];  // 진단기가 전송

if(key == 0x5678u)  // 검증!
{
    stBswDcmData.stDiagStatus.isSecurityUnlock = TRUE;
}
```

**질문 4-1:**
CAN 데이터가 다음이라면, 조립되는 key 값은?

```
diagDataBufferRx[2] = 0x56
diagDataBufferRx[3] = 0x78
```

계산: `(0x56 << 8) | 0x78 = ???`

**질문 4-2:**
만약 정답이 0x5678인데, 진단기에서 0x1111을 보냈다면?

다음 중 어떤 응답을 받을까?

A) [0x02][0x67][0x02][...] - 성공  
B) [0x03][0x7F][0x27][0x35][...] - NRC_InvalidKey  
C) ECU가 리셋됨  
D) 아무 응답 안 함  

**질문 4-3:**
SecurityAccess가 성공했을 때 어떤 상태 변수가 TRUE로 바뀌나?

A) isEcuReset  
B) isSecurityUnlock  
C) currentSession  
D) isServiceRequest  

---

## 📝 문제 5: 코드 흐름 추적

**다음 시나리오를 단계별로 추적하세요:**

```
초기 상태:
- currentSession = DIAG_SESSION_EXTENDED ✅ (이미 전환된 상태)
- isSecurityUnlock = FALSE

입력: RequestSeed 요청
diagReq[] = [0x02, 0x27, 0x01, 0x55, 0x55, 0x55, 0x55, 0x55]

단계 1) REoiBswDcm_pDcmCmd_processDiagRequest() 실행
```

**질문 5-1:**
processDiagRequest()에서 diagDataBufferRx에 복사되는 데이터는?

```
diagDataBufferRx[0] = ???
diagDataBufferRx[1] = ???
길이(diagDataLengthRx) = ???
```

---

```
단계 2) REtBswDcm_bswDcmMain() 실행
        serviceId 추출 및 securityAccess() 호출
```

**질문 5-2:**
securityAccess() 함수가 먼저 하는 일은?

```c
if(stBswDcmData.stDiagStatus.currentSession == DIAG_SESSION_DEFAULT)
{
    errorResult = ??? ;
}
```

빈칸을 채우고, 이것이 왜 필요한지 설명하세요.

---

```
단계 3) RequestSeed 처리
```

**질문 5-3:**
RequestSeed(SubFunc=0x01)에서 먼저 체크하는 것은?

```c
if(stBswDcmData.diagDataLengthRx != ???)  // ← 몇 바이트여야 함?
{
    errorResult = NRC_IncorrectMessageLengthOrInvalidFormat;
}
```

---

**질문 5-4:**
RequestSeed 성공 시 생성되는 응답 메시지를 채워보세요:

```
diagDataBufferTx[0] = ???  // 프레임 정보
diagDataBufferTx[1] = ???  // SID + 0x40
diagDataBufferTx[2] = ???  // SubFunc 에코
diagDataBufferTx[3] = ???  // Seed High Byte
diagDataBufferTx[4] = ???  // Seed Low Byte
```

hint: seed = 0x1234

---

```
단계 4) Seed를 받은 진단기가 Key를 계산해서 다시 요청
        (실제로는 진단기의 일)
        
단계 5) SendKey 처리
```

**질문 5-5:**
SendKey(SubFunc=0x02) 요청에서 먼저 체크하는 것은?

```c
if(stBswDcmData.diagDataLengthRx != ???)  // ← 몇 바이트여야 함?
{
    errorResult = NRC_IncorrectMessageLengthOrInvalidFormat;
}
```

---

**질문 5-6:**
Key 비교 로직을 분석하세요:

```c
key = ((uint16)stBswDcmData.diagDataBufferRx[2] << 8u) 
    | stBswDcmData.diagDataBufferRx[3];

if(key == 0x5678u)
{
    // ✅ 성공 응답 생성
    stBswDcmData.diagDataBufferTx[0] = ???
    stBswDcmData.diagDataBufferTx[1] = ???
    stBswDcmData.diagDataBufferTx[2] = ???
    stBswDcmData.stDiagStatus.isSecurityUnlock = TRUE;
}
else
{
    // ❌ 실패
    errorResult = ???
}
```

**6-1) 빈칸을 채우세요 (성공 응답)**

**6-2) 실패 시 errorResult에 올 NRC 코드는?**

A) 0x13 (IncorrectMessageLength)  
B) 0x35 (InvalidKey)  
C) 0x7E (ServiceNotSupportedInActiveSession)  
D) 0x12 (SubFunctionNotSupported)  

---

```
단계 6) 최종 응답 전송
```

**질문 5-7:**
SecurityAccess 인증이 완료되면, **다음에 가능해지는 서비스**는 뭘까?

A) ECUReset(0x11)  
B) DiagSessionControl(0x10)  
C) WriteDataByIdentifier(0x2E) ← 펌웨어 쓰기!  
D) 모든 서비스  

---

## 📝 문제 6: OTA 플로우 통합

**완전한 OTA 시퀀스를 순서대로 배열하세요:**

```
A) SecurityAccess (Seed/Key 인증)
B) DiagSessionControl (Default → Extended)
C) WriteDataByIdentifier (펌웨어 저장)
D) ECUReset (새 펌웨어 적용)
E) 부팅 시작 (새 펌웨어 실행)

올바른 순서는? ____ → ____ → ____ → ____ → ____
```

---

---

---

# 🎯 답 & 해설

## 문제 1 정답: SecurityAccess의 필요성

**정답: B) 누구나 펌웨어를 수정할 수 있는 게 아니라, 인증된 진단기만 할 수 있도록 제한하기 위해**

### 해설

OTA 보안의 핵심:

```
진단기 유형:

[1] 공개된 진단기 (정비소)
    → 재부팅, 상태 확인 (Default 세션)
    → 펌웨어 수정 불가 ✗

[2] 공식 진단기 (제조사)
    → 펌웨어 업데이트 가능 (Extended + SecurityAccess)
    → isSecurityUnlock = TRUE ✅
```

**만약 SecurityAccess가 없으면?**
```
누구나 펌웨어 수정 가능
→ 악의적 코드 주입 가능
→ 차량 보안 위협! 🚨
```

**SecurityAccess의 역할:**
```
1. Default 세션: 누구나 진단 가능
2. Extended 세션: 도움말 기능만
3. Extended + SecurityAccess: 펌웨어 수정 권한 ✅

즉, 3단계 보안!
```

**참고:** 강의록 **P.28 "Uds : SecurityAccess"**: 메시지 정의, 이미지 `28_image_0.png`

---

## 문제 2 정답: Seed/Key 2단계 메커니즘

### 2-1 0x27과 0x01은?

**답:**
- `0x27`: **SecurityAccess 서비스 ID**
- `0x01`: **SubFunc = RequestSeed** (Seed 요청)

---

### 2-2 B단계의 0x12, 0x34는?

**답: Seed 값**

```
ECU가 생성한 Seed = 0x1234
↓
두 바이트로 분해:
High Byte = 0x12
Low Byte = 0x34

응답: [0x04][0x67][0x01][0x12][0x34][0xAA][0xAA][0xAA]
                             ↑ Seed
```

**의미:**
- Seed는 ECU가 **무작위** 생성 (실제로는, UDS_3은 고정값)
- 진단기가 이 Seed를 받아서 Key를 계산

**참고:** 강의록 **P.28 "Table 40"**: RequestSeed 응답 정의

---

### 2-3 C단계에서 진단기가 한 일

**답: 선택지 D) 어디서 받은 값을 그냥 하드코딩된 Key(0x5678)로 보냄**

```
실제 OTA_3는 "Fake Security"😅

정상적인 Seed/Key:
  Seed(0x1234) + MasterPassword → 계산 → Key(결과)
  
UDS_3의 현실:
  Seed(0x1234) → 무시하고 → 하드코딩 Key(0x5678) 전송 😅
```

**이유:**
- UDS_3은 **교육용** (간단하게)
- 실제 제품: Seed마다 다른 Key 계산
- UDS_4 ~ UDS_7: 실제 테이블 기반 인증 추가

**참고:** 강의록 **P.30 "Table - Seed/Key"**: 고정값 예시

---

### 2-4 D단계의 0x67 의미

**답: SecurityAccess에 대한 긍정 응답 (인증 성공)**

```
메시지 규칙:
- 요청 SID: 0x27
- 긍정 응답: 0x27 + 0x40 = 0x67 ✅
- 부정 응답: 0x7F

[0x02][0x67][0x02][0xAA][...]
      ↑
    SID + 0x40 = 긍정!
```

**의미:**
- ECU: "Key가 맞다! 인증 완료 ✅"
- 진단기: 이제 WriteData(0x2E) 가능

---

## 문제 3 정답: 세션 제약

**정답: B) 펌웨어 수정은 Extended(확장) 세션에서만 가능한데, 사전에 인증을 못 하는 이유가 없으니까 제약**

### 해설

보안 계층 구조:

```
[1] Default 세션 (공개)
    └─ DiagSessionControl ✓
    └─ ECUReset ✓
    └─ SecurityAccess ✗ ← 불필요! Extended가 아니니까

[2] Extended 세션 (진단 기능)
    └─ SecurityAccess ✓ ← 여기서만!
    └─ WriteData (인증 후) ✓

논리:
- Default → 공개 정보만
- Extended → 진단 기능, 하지만 아직 수정은 안 됨
- Extended + SecurityAccess → 수정 권한 ✓
```

**실제 시나리오:**
```
정비사: [Default] 상태 확인만
        ↓
제조사: [Extended] 진단 기능 추가
        ↓ SecurityAccess
        [Unlock] 펌웨어 수정 권한 ✅
```

**참고:** 강의록 **P.6 "Table 23"**: 서비스별 세션 제약 표, 이미지 `10_image_0.png`

---

## 문제 4 정답: Seed와 Key 이해

### 4-1 Key 조립 계산

**답:**

```
diagDataBufferRx[2] = 0x56
diagDataBufferRx[3] = 0x78

(0x56 << 8) | 0x78
= 0x5600 | 0x0078
= 0x5678 ✅
```

**비트 분석:**
```
0x56 = 0101 0110
<< 8 = 0101 0110 0000 0000
       ↓ 이게 High Byte

0x78 = 0000 0000 0111 1000
       ↓ 이게 Low Byte

OR합  = 0101 0110 0111 1000 = 0x5678 ✓
```

---

### 4-2 정답이 아닌 Key를 보냈을 때

**정답: B) [0x03][0x7F][0x27][0x35][...] - NRC_InvalidKey**

```c
if(key == 0x5678u)
{
    // 성공
}
else
{
    errorResult = NRC_InvalidKey;  // 0x35
}

// 에러 응답:
diagDataBufferTx[0] = 0x03
diagDataBufferTx[1] = 0x7F       // Negative Response
diagDataBufferTx[2] = 0x27       // 원본 SID
diagDataBufferTx[3] = 0x35       // NRC_InvalidKey
```

**진단기가 알게 되는 것:**
- "잘못된 Key야! 다시 계산해"
- 또는 연속 실패 → "공격 탐지"

---

### 4-3 성공 시 TRUE로 바뀌는 변수

**정답: B) isSecurityUnlock**

```c
stBswDcmData.stDiagStatus.isSecurityUnlock = TRUE;
//                      ↑
//          ST_DiagStatus 구조체의 멤버
```

**다음 서비스가 확인:**
```c
// WriteData(0x2E)에서
if(stDiagStatus.isSecurityUnlock == TRUE)
{
    // 펌웨어 쓰기 가능 ✅
}
```

**참고:** 강의록 **P.28 코드**: SecurityAccess 구현

---

## 문제 5 정답: 코드 흐름 추적

### 5-1 diagDataBufferRx에 복사되는 데이터

**답:**

```
입력: diagReq[] = [0x02, 0x27, 0x01, 0x55, 0x55, 0x55, 0x55, 0x55]
                   ↑     ↑     ↑
                 프레임  SID  SubFunc

memcpy(diagDataBufferRx, &diagReq[1], diagDataLengthRx)
                         ↑ diagReq[1]부터 복사
```

따라서:
- `diagDataBufferRx[0] = 0x27` (SID)
- `diagDataBufferRx[1] = 0x01` (SubFunc)
- `diagDataLengthRx = 2` (바이트)

---

### 5-2 세션 확인의 필요성

**답:**

```c
errorResult = NRC_ServiceNotSupportedInActiveSession;  // 0x7E
```

**필요성:**
```
SecurityAccess는 Extended 세션에서만!

만약 Default 세션에서 요청하면:
→ NRC_0x7E 반환
→ 진단기: "Extended로 먼저 바꿔!"
→ DiagSessionControl 실행 후 재시도
```

**설계 의도:**
- 의도하지 않은 인증 방지
- 프로토콜 위반 감지

---

### 5-3 RequestSeed 길이 체크

**답: 2**

```c
if(stBswDcmData.diagDataLengthRx != 2u)
```

**이유:**
```
RequestSeed 요청: [0x27][0x01] = 2바이트
            ├─ SID(0x27)
            └─ SubFunc(0x01)

CAN에서: [0x02][0x27][0x01][Padding...]
          ↑     1바이트 + 1바이트 = 합계 2
```

**다른 길이면 NRC_0x13 반환**

---

### 5-4 RequestSeed 응답 구성

**답:**

```c
diagDataBufferTx[0] = (FRAME_TYPE_SINGLE << 4u) | 4u
                    = (0 << 4) | 4
                    = 0x04  // SINGLE 프레임, 길이 4

diagDataBufferTx[1] = SID_SecurityAccess + 0x40u
                    = 0x27 + 0x40
                    = 0x67  // 긍정 응답

diagDataBufferTx[2] = 0x01  // SubFunc 에코 (RequestSeed)

diagDataBufferTx[3] = (uint8)((seed & 0xFF00u) >> 8u)
                    = (0x1234 & 0xFF00) >> 8
                    = 0x1200 >> 8
                    = 0x12  // High Byte

diagDataBufferTx[4] = (uint8)(seed & 0xFFu)
                    = 0x1234 & 0xFF
                    = 0x34  // Low Byte
```

**최종 응답:**
```
[0x04] [0x67] [0x01] [0x12] [0x34] [0xAA] [0xAA] [0xAA]
 길이   SID+40  SubFunc Seed_Hi Seed_Lo  Filler
```

**참고:** 강의록 **P.28 "Table 44"**: 응답 메시지 정의

---

### 5-5 SendKey 길이 체크

**답: 4**

```c
if(stBswDcmData.diagDataLengthRx != 4u)
```

**이유:**
```
SendKey 요청: [0x27][0x02][Key_Hi][Key_Lo] = 4바이트
             ├─ SID(0x27)
             ├─ SubFunc(0x02)
             └─ Key(2바이트)

CAN에서: [0x04][0x27][0x02][0x56][0x78][Padding...]
          ↑     1 + 1 + 2 = 합계 4
```

**다른 길이면 NRC_0x13 반환**

---

### 5-6 성공 응답 생성 & NRC

**6-1) 성공 응답:**

```c
stBswDcmData.diagDataBufferTx[0] = (FRAME_TYPE_SINGLE << 4u) | 2u
                                 = 0x02  // SINGLE, 길이 2

stBswDcmData.diagDataBufferTx[1] = SID_SecurityAccess + 0x40u
                                 = 0x67  // 긍정 응답

stBswDcmData.diagDataBufferTx[2] = 0x02  // SubFunc 에코
```

**최종:**
```
[0x02] [0x67] [0x02] [0xAA] [0xAA] [0xAA] [0xAA] [0xAA]
 길이   SID+40  SubFunc  Filler
```

**6-2) 실패 시 NRC:**

**정답: B) 0x35 (InvalidKey)**

```c
else
{
    errorResult = NRC_InvalidKey;  // 0x35
}
```

---

### 5-7 다음에 가능해지는 서비스

**정답: C) WriteDataByIdentifier(0x2E)**

```
인증 전:
  isSecurityUnlock = FALSE
  → WriteData 불가능 ✗

인증 후:
  isSecurityUnlock = TRUE
  → WriteData 가능 ✅ (펌웨어 저장!)
```

**3단계 보안 완성:**
```
[1] Extended 세션
[2] SecurityAccess 완료
[3] WriteData로 펌웨어 수정 ← 이제 가능!
```

**참고:** 강의록 **P.28 코드**: SecurityAccess 플래그 설정

---

## 문제 6 정답: OTA 플로우 통합

**답: B → A → C → D → E**

```
B) DiagSessionControl (Default → Extended)
   ↓ "Extended 세션으로 전환"
   
A) SecurityAccess (Seed/Key 인증)
   ↓ "너는 정말 공식 진단기니?"
   
C) WriteDataByIdentifier (펌웨어 저장)
   ↓ "새 펌웨어를 Flash/NVM에 저장"
   
D) ECUReset (새 펌웨어 적용)  
   ↓ "CPU를 재부팅해서 새 펌웨어 로드"
   
E) 부팅 시작 (새 펌웨어 실행)
   🎉 "완료!"
```

**상세 설명:**

| 단계 | 서비스 | 세션 | 상태 | 목적 |
|------|--------|------|------|------|
| 1 | DiagSessionControl | Default → Extended | currentSession=1 | 진단 기능 활성화 |
| 2 | SecurityAccess | Extended | isSecurityUnlock=1 | 펌웨어 수정 권한 획득 |
| 3 | WriteData | Extended + Auth | 데이터 저장 | 새 펌웨어 저장 |
| 4 | ECUReset | 무관 | 리셋 실행 | 하드웨어 재부팅 |
| 5 | Boot | 무관 | 새 펌웨어 실행 | OTA 완료 ✅ |

**참고:** 강의록 **P.30 "Practice"**: 실제 OTA 시나리오

---

## 📊 채점 기준

| 문제 | 만점 | 난이도 | 핵심 개념 |
|------|------|--------|---------|
| 1 | 1점 | ⭐ 쉬움 | SecurityAccess의 목적 |
| 2 | 4점 | ⭐⭐ 중간 | Seed/Key 2단계 메커니즘 |
| 3 | 1점 | ⭐⭐⭐ 어려움 | 세션 제약 이해 |
| 4 | 3점 | ⭐⭐ 중간 | 비트 연산, 상태 변수 |
| 5 | 8점 | ⭐⭐⭐⭐ 매우 어려움 | 전체 흐름 추적 |
| 6 | 1점 | ⭐ 쉬움 | OTA 전체 플로우 |
| **합계** | **18점** |  |  |

---

## 🎓 결과 해석

```
17-18점: 완벽! UDS_4로 진행 가능 🚀
14-16점: 우수! 조금 더 보면 더 좋지만 OK 📖
11-13점: 양호! "Fake Security" 개념과 테이블 기반 인증(UDS_4) 다시 보기 🔄
8-10점:  보통! UDS_3_Analysis의 "2️⃣ 메시지 프로토콜", "5️⃣ 핵심 개념" 재읽기 🐢
5-7점:   아직도! P.28-30 강의록 직접 읽기 + 코드 다시 추적 📚
0-4점:   다시 한 번! SecurityAccess가 뭔지부터 학습 필요 🎓
```

---

## 🔗 UDS 서비스 비교 (1~3)

| 항목 | UDS_1 (0x10) | UDS_2 (0x11) | UDS_3 (0x27) |
|------|--------------|--------------|--------------|
| **명칭** | DiagSessionControl | ECUReset | SecurityAccess |
| **SubFunc** | 0x01(Default), 0x03(Extended) | 0x01(HardReset) | 0x01(Seed), 0x02(Key) |
| **응답** | 0x50 + P2/P2* | 0x51 | 0x67 |
| **응답 길이** | 6바이트 | 2바이트 | 4바이트 or 2바이트 |
| **목적** | 세션 전환 | 펌웨어 적용 | 인증 |
| **세션** | 양쪽 O | 양쪽 O | Extended만 |
| **상태 변수** | currentSession | isEcuReset | isSecurityUnlock |
| **OTA 순서** | 1단계 | 마지막(4) | 2단계(인증) |

---

**점수 나왔으면 말씀해주세요! 그리고 어려웠던 부분이 있으면 설명해드릴 수 있습니다.** 😊
