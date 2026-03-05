# UDS_5 (CommunicationControl 0x28) 퀴즈

> UDS_5_Analysis.md를 읽으며 학습한 내용을 체크하세요.
> 먼저 **문제 1~5번**을 모두 풀고, 답은 뒤에서 확인하세요!

---

## 📝 문제 1: CommunicationControl의 역할

**CommunicationControl(0x28)이 OTA에서 왜 중요한가?**

다음 중 가장 적절한 답을 고르세요:

A) ECU를 더 빠르게 하기 위해  
B) Flash 쓰기 중 CAN 메시지 간섭 차단 → 펌웨어 무결성 보호  
C) 진단기와의 통신을 암호화하기 위해  
D) ECU의 전력 소비를 줄이기 위해  

---

## 📝 문제 2: 요청/응답 포맷 분석

**다음 CAN 메시지를 분석하세요:**

```
[TX] 7D3: 03 28 03 01 AA AA AA AA  (Disable 요청)
[RX] 7DB: 02 68 03 AA AA AA AA AA  (응답)
```

**질문:**

**(2-1) 첫 번째 바이트 0x03과 0x02는 각각 뭘 의미하나?**

**(2-2) 0x28과 0x68의 관계는 뭘까? (어떤 규칙인가?)**

**(2-3) [TX] 세 번째 바이트 0x03은 뭘 의미하나?**
- Subfuntion? Communication Type? Frame Type?

**(2-4) [TX] 네 번째 바이트 0x01은 뭘 의미하나?**
- Subfuntion? Communication Type? Frame Type?

**(2-5) 왜 응답은 2바이트인데 요청은 3바이트인가?**

---

## 📝 문제 3: UDS_4 vs UDS_5 차이점

**UDS_4(SecurityAccess 0x27)와 UDS_5(CommunicationControl 0x28)를 비교하세요.**

다음 표를 완성하세요:

| 항목 | UDS_4 | UDS_5 |
|------|-------|-------|
| **Service ID** | 0x27 | ??? |
| **필요한 세션** | ??? | ExtendedSession |
| **요청 길이** | 2바이트 | ??? |
| **응답 길이** | 4바이트 (Seed 포함) | ??? |
| **핵심 상태 플래그** | isSecurityUnlock | ??? |
| **파라미터 여부** | 없음 (SubFunc만) | 있음 (CommType) |

---

## 📝 문제 4: 세션 제약 및 에러 처리

**다음 상황에서 어떻게 될까?**

**(4-1) Default 세션에서 CommunicationControl 요청**

성공? / 실패?

만약 실패라면 어떤 에러코드가 반환될까?
- NRC_ServiceNotSupported (0x11)
- NRC_SubFunctionNotSupportedInActiveSession (0x22)
- NRC_IncorrectMessageLengthOrInvalidFormat (0x13)

**(4-2) ExtendedSession에서 요청 길이가 2바이트인 경우**

```
[TX] 7D3: 02 28 03 AA AA AA AA AA
```

성공? / 실패?

만약 실패라면 어떤 에러코드가 반환될까?

**(4-3) ExtendedSession에서 Communication Type이 0xFF인 경우**

```
[TX] 7D3: 03 28 03 FF AA AA AA AA
```

성공? / 실패?

만약 실패라면 어떤 에러코드가 반환될까?
- NRC_RequestOutRange (0x31)
- NRC_SubFunctionNotSupported (0x12)
- NRC_IncorrectMessageLengthOrInvalidFormat (0x13)

**(4-4) SFID_CommunicationControl_enableRxAndTx (0x00) 요청시 isCommunicationDisable 플래그**

실행 후 예상 값: TRUE? / FALSE?

---

## 📝 문제 5: OTA 흐름 및 코드 추적

**OTA 진행 중 다음 시나리오를 추적하세요.**

진단기가 Disable 명령을 보냅니다:

```
입력:  isPhysical=TRUE
       diagReq[] = [0x03, 0x28, 0x03, 0x01, 0x55, 0x55, 0x55, 0x55]
       (CAN에서 받은 원본 데이터)
```

```
단계 1) REoiBswDcm_pDcmCmd_processDiagRequest() 실행
```

**질문 5-1:**
```c
uint8_t frameType = diagReq[0] >> 4u;
```
frameType의 값은?

**질문 5-2:**
```c
stBswDcmData.diagDataLengthRx = (diagReq[0] & 0x0Fu);
```
diagDataLengthRx의 값은?

**질문 5-3:**
```c
(void)memcpy(stBswDcmData.diagDataBufferRx, &diagReq[1], stBswDcmData.diagDataLengthRx);
```
diagDataBufferRx[0], diagDataBufferRx[1], diagDataBufferRx[2]의 값은?

---

```
단계 2) REtBswDcm_bswDcmMain() 실행
```

**질문 5-4:**
```c
serviceId = stBswDcmData.diagDataBufferRx[0];
```
serviceId의 값은?

**질문 5-5:**
```c
uint8 subFunctionId = stBswDcmData.diagDataBufferRx[1] & 0x7Fu;
uint8 communicationType = stBswDcmData.diagDataBufferRx[2];
```
subFunctionId와 communicationType의 값은?

---

```
단계 3) communicationControl() 함수
```

**질문 5-6:**
이 함수가 성공했을 때 stBswDcmData.stDiagStatus.isCommunicationDisable은?

TRUE? / FALSE?

**질문 5-7:**
생성되는 응답 메시지를 채워보세요:

```
diagDataBufferTx[0] = ???  (프레임 타입|길이)
diagDataBufferTx[1] = ???  (서비스 응답 코드)
diagDataBufferTx[2] = ???  (서브함수 에코)
```

**질문 5-8:**
최종 CAN TX 메시지 (패딩 포함) 예상 값:

```
[7DB] ?? ?? ?? ?? ?? ?? ?? ??
```

---

---

---

# 🎯 답 & 해설

## 문제 1 정답: CommunicationControl의 역할

**정답: B) Flash 쓰기 중 CAN 메시지 간섭 차단 → 펌웨어 무결성 보호**

### 해설

CommunicationControl(0x28)은 **OTA의 핵심 기능**입니다:

```
[OTA 흐름]
① DiagSessionControl → ExtendedSession 
② SecurityAccess → ECU 인증
③ CommunicationControl(Disable) ← ✅ 통신 OFF!
④ [Flash 쓰기 진행 - CAN 메시지 차단]
⑤ CommunicationControl(Enable) ← ✅ 통신 ON!
⑥ EcuReset → 재부팅
```

- Flash 메모리 쓰기 중에 CAN 메시지가 들어오면 **ECU가 distracted**됨
- 펌웨어 쓰기가 중단되거나 corrupted될 수 있음
- 따라서 미리 통신을 우아하게 차단해야 함

---

## 문제 2 정답: 요청/응답 포맷 분석

### 2-1) 첫 번째 바이트 0x03과 0x02의 의미

| 메시지 | 0x03 | 0x02 | 의미 |
|--------|------|------|------|
| **[TX] 요청** | 0x03 = 길이 3바이트 | - | SID + SubFunc + CommType = 3바이트 |
| **[RX] 응답** | - | 0x02 = 길이 2바이트 | SID + SubFunc = 2바이트 (CommType 불필요) |

**프레임 구조:**
```
Byte 0: [FrameType(4bit) | Length(4bit)]
         0x03 = [0000 | 0011] = SingleFrame, 3바이트
         0x02 = [0000 | 0010] = SingleFrame, 2바이트
```

### 2-2) 0x28과 0x68의 관계

**ISO 14229 규칙:**
```
Response SID = Request SID + 0x40
0x28 + 0x40 = 0x68
```

따라서 0x68 = CommunicationControl의 **긍정 응답 코드**

### 2-3) [TX] 세 번째 바이트 0x03

**SubFunction** = 0x03 = `SFID_CommunicationControl_disableRxAndTx`

```c
#define SFID_CommunicationControl_enableRxAndTx   0x00u   // Enable
#define SFID_CommunicationControl_disableRxAndTx  0x03u   // Disable ← 요청은 0x03
```

### 2-4) [TX] 네 번째 바이트 0x01

**Communication Type** = 0x01 = `CommunicationType_normalCommunicationMessages`

```c
#define CommunicationType_normalCommunicationMessages  0x01u
```

이것이 **필수 파라미터**입니다. 어떤 종류의 통신을 제어할지 지정합니다.

### 2-5) 요청 3바이트 vs 응답 2바이트

| 방향 | 길이 | 포함 데이터 | 이유 |
|------|------|----------|------|
| **요청** | 3바이트 | SID + SubFunc + **CommType** | ECU가 어떤 통신을 제어할지 알아야 함 |
| **응답** | 2바이트 | SID + SubFunc | CommType은 이미 받았으니 echo하면 됨 |

---

## 문제 3 정답: UDS_4 vs UDS_5 비교표

| 항목 | UDS_4 | UDS_5 |
|------|-------|-------|
| **Service ID** | 0x27 | **0x28** |
| **필요한 세션** | ExtendedSession | **ExtendedSession** |
| **요청 길이** | 2바이트 | **3바이트** ← KEY DIFFERENCE! |
| **응답 길이** | 4바이트 (Seed 포함) | **2바이트** |
| **핵심 상태 플래그** | isSecurityUnlock | **isCommunicationDisable** |
| **기본 차이** | 인증 메커니즘 | **통신 제어** |

**핵심 포인트:**
- UDS_4: 보안 인증 (2-stage: Seed→Key)
- UDS_5: 네트워크 제어 (Enable/Disable)

---

## 문제 4 정답: 세션 제약 및 에러 처리

### 4-1) Default 세션에서 CommunicationControl 요청

**실패**

**에러코드: NRC_SubFunctionNotSupportedInActiveSession (0x22)**

```c
else if(stBswDcmData.stDiagStatus.currentSession != DIAG_SESSION_EXTENDED)
{
    errorResult = NRC_SubFunctionNotSupportedInActiveSession;  // ← 이 에러!
}
```

**다만, 함수명이 "SubFunctionNotSupported"지만 실제로는 세션 검증**입니다.

### 4-2) ExtendedSession에서 길이가 2바이트

**실패**

**에러코드: NRC_IncorrectMessageLengthOrInvalidFormat (0x13)**

```c
if(stBswDcmData.diagDataLengthRx != 3u)  // 정확히 3바이트만 허용!
{
    errorResult = NRC_IncorrectMessageLengthOrInvalidFormat;  // ← 이 에러!
}
```

**UDS_5는 반드시 3바이트여야 하는 엄격한 검증**이 있습니다.

### 4-3) ExtendedSession, CommType = 0xFF

**실패**

**에러코드: NRC_RequestOutRange (0x31)**

```c
else if(communicationType != CommunicationType_normalCommunicationMessages)  // 0x01만 허용
{
    errorResult = NRC_RequestOutRange;  // ← 이 에러!
}
```

**0x01 외의 CommType은 모두 거부**됩니다. 향후 확장을 위한 설계로 보입니다.

### 4-4) enableRxAndTx (0x00) 요청 후 플래그

**FALSE**

```c
if(subFunctionId == SFID_CommunicationControl_enableRxAndTx)
{
    stBswDcmData.stDiagStatus.isCommunicationDisable = FALSE;  // ← 통신 활성화!
}
```

- **isCommunicationDisable = TRUE** → 통신 차단 중
- **isCommunicationDisable = FALSE** → 통신 정상 (또는 재개)

---

## 문제 5 정답: OTA 흐름 및 코드 추적

### 입력 분석

```
diagReq[0] = 0x03  (FrameType:0, Length:3)
diagReq[1] = 0x28  (SID)
diagReq[2] = 0x03  (SubFunc: Disable)
diagReq[3] = 0x01  (CommType: Normal)
```

### 5-1) frameType 값

```c
uint8_t frameType = diagReq[0] >> 4u;  // 0x03 >> 4 = 0x00
```

**답: 0x00** = SingleFrame

### 5-2) diagDataLengthRx 값

```c
stBswDcmData.diagDataLengthRx = (diagReq[0] & 0x0Fu);  // 0x03 & 0x0F = 0x03
```

**답: 0x03** (3바이트)

### 5-3) diagDataBufferRx 복사 내용

```c
(void)memcpy(stBswDcmData.diagDataBufferRx, &diagReq[1], stBswDcmData.diagDataLengthRx);
// diagReq[1]부터 3바이트 복사
```

```
diagDataBufferRx[0] = 0x28  (SID)
diagDataBufferRx[1] = 0x03  (SubFunc)
diagDataBufferRx[2] = 0x01  (CommType)
```

### 5-4) serviceId 값

```c
serviceId = stBswDcmData.diagDataBufferRx[0];  // = 0x28
```

**답: 0x28** = SID_CommunicationControl

### 5-5) subFunctionId와 communicationType

```c
uint8 subFunctionId = stBswDcmData.diagDataBufferRx[1] & 0x7Fu;  // 0x03 & 0x7F = 0x03
uint8 communicationType = stBswDcmData.diagDataBufferRx[2];      // = 0x01
```

```
subFunctionId = 0x03  (disableRxAndTx)
communicationType = 0x01  (normalCommunicationMessages)
```

### 5-6) disableRxAndTx 후 isCommunicationDisable 플래그

```c
else if(subFunctionId == SFID_CommunicationControl_disableRxAndTx)  // 0x03
{
    stBswDcmData.stDiagStatus.isCommunicationDisable = TRUE;  // ← Disable!
}
```

**답: TRUE** (통신 비활성화됨)

### 5-7) 응답 메시지 구성

```c
stBswDcmData.diagDataLengthTx = 2u;
stBswDcmData.diagDataBufferTx[0] = (FRAME_TYPE_SINGLE << 4u) | stBswDcmData.diagDataLengthTx;
                                  = (0 << 4) | 2 = 0x02
stBswDcmData.diagDataBufferTx[1] = SID_CommunicationControl + 0x40u;
                                  = 0x28 + 0x40 = 0x68
stBswDcmData.diagDataBufferTx[2] = subFunctionId;
                                  = 0x03
```

```
diagDataBufferTx[0] = 0x02  (Frame:Single, Length:2)
diagDataBufferTx[1] = 0x68  (Response SID)
diagDataBufferTx[2] = 0x03  (SubFunc Echo)
```

### 5-8) 최종 CAN TX 메시지

패딩 처리:
```c
for(uint8 i=stBswDcmData.diagDataLengthTx+1u; i<TX_MSG_LEN_PHY_RES; i++)
{
    stBswDcmData.diagDataBufferTx[i] = REF_VALUE_FILLER_BYTE;  // 0xAA
}
```

```
[7DB] 02 68 03 AA AA AA AA AA
      └─┬─┘ └─────────────┘
      응답  패딩 (0xAA)
```

**답: [7DB] 02 68 03 AA AA AA AA AA**

---

## 📊 학습 검증

**Level 1 (기본):**
- 문제 1, 2-1, 4-1 정답 → CommunicationControl 기본 개념 OK ✅

**Level 2 (논리):**
- 문제 2, 3, 4-2~4-4 정답 → 프로토콜 설계 논리 이해 OK ✅

**Level 3 (고급):**
- 문제 5 전부 정답 → 하드웨어 수준의 메시지 추적 마스터 ✅

---

## 💡 핵심 복습

| 개념 | 정리 |
|------|------|
| **요청 길이** | 3바이트 (SID + SubFunc + CommType) |
| **응답 길이** | 2바이트 (SID + SubFunc) |
| **Response SID** | Request SID + 0x40 = 0x68 |
| **필수 세션** | ExtendedSession만 가능 |
| **핵심 플래그** | isCommunicationDisable (TRUE=차단, FALSE=활성) |
| **CommType** | 0x01만 지원 (normalCommunicationMessages) |
| **예외 처리** | 길이, 세션, CommType 3중 검증 |
| **OTA 역할** | Flash 쓰기 중 CAN 간섭 차단 |
