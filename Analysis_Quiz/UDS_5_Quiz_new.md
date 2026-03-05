# UDS_5 (CommunicationControl 0x28) 퀴즈

> UDS_5_Analysis.md를 읽으며 학습한 내용을 체크하세요.
> **문제 1~5번**을 모두 풀고, 뒤에서 답을 확인하세요!

---

## 📝 문제 1: CommunicationControl의 OTA 역할

**UDS_5가 OTA에서 왜 필수인지 설명하세요.**

다음 중 가장 정확한 답을 고르세요:

A) ECU를 더 빠르게 부팅하기 위해  
B) Flash 쓰기 중 CAN 메시지 간섭 방지 → 펌웨어 무결성 보호  
C) 진단기와 암호화 통신하기 위해  
D) ECU 전력 소비 절약  

---

## 📝 문제 2: 요청/응답 메시지 포맷 분석

**다음 CAN 메시지를 분석하세요:**

```
[TX] 7D3: 03 28 03 01 AA AA AA AA  (CommunicationControl Disable 요청)
[RX] 7DB: 02 68 03 AA AA AA AA AA  (응답)
```

**질문:**

**(2-1) 첫 바이트 0x03과 0x02는 각각 무엇을 의미하나?**

**(2-2) 요청의 0x28과 0x68의 관계는?**

**(2-3) 요청의 0x03은 뭐고, 0x01은 뭐인가?**

**(2-4) 요청은 4바이트(0x03+3바이트)인데 응답은 2바이트인 이유는?**

---

## 📝 문제 3: 세션과 파라미터 검증

**다음 시나리오를 판단하세요:**

**(3-1) DefaultSession에서 CommunicationControl 요청**

성공? / 실패?

만약 실패라면 에러코드는?
- NRC_ServiceNotSupported
- NRC_SubFunctionNotSupportedInActiveSession
- NRC_IncorrectMessageLengthOrInvalidFormat

**(3-2) ExtendedSession, 요청 길이 2바이트**

```
[요청] 02 28 03 AA AA AA AA AA
```

성공? / 실패?

**(3-3) ExtendedSession, CommType = 0xFF**

```
[요청] 03 28 03 FF AA AA AA AA
```

성공? / 실패?

에러코드는?

---

## 📝 문제 4: SubFunction과 상태 플래그

**다음을 완성하세요:**

| SubFunc | 값 | 의미 | 결과: isCommunicationDisable |
|---------|-----|------|---|
| enableRxAndTx | ??? | ??? | ??? |
| disableRxAndTx | 0x03 | ??? | TRUE |

---

## 📝 문제 5: 코드 흐름 추적

**Disable 요청이 들어온 상황:**

```
입력: isPhysical=TRUE
      diagReq[] = [0x03, 0x28, 0x03, 0x01, 0x55, 0x55, 0x55, 0x55]
```

```
단계 1) REoiBswDcm_pDcmCmd_processDiagRequest() 실행
```

**질문 5-1:**
```c
stBswDcmData.diagDataLengthRx = (diagReq[0] & 0x0Fu);
```
diagDataLengthRx = ?

**질문 5-2:**
```c
(void)memcpy(stBswDcmData.diagDataBufferRx, &diagReq[1], diagDataLengthRx);
```
diagDataBufferRx[0], [1], [2]의 값은?

---

```
단계 2) REtBswDcm_bswDcmMain() → communicationControl() 실행
```

**질문 5-3:**
```c
uint8 subFunctionId = stBswDcmData.diagDataBufferRx[1] & 0x7Fu;
uint8 communicationType = stBswDcmData.diagDataBufferRx[2];
```
각각의 값은?

**질문 5-4:**
3단계 검증 중 disableRxAndTx (SubFunc 0x03)이 모두 통과하면:

```c
stBswDcmData.stDiagStatus.isCommunicationDisable = ???
```
TRUE? / FALSE?

**질문 5-5:**
생성되는 응답 메시지:

```
diagDataBufferTx[0] = ???
diagDataBufferTx[1] = ???
diagDataBufferTx[2] = ???
```

**질문 5-6:**
최종 CAN TX 메시지:

```
[7DB] ?? ?? ?? ?? ?? ?? ?? ??
```

---

---

---

# 🎯 답 & 해설

## 문제 1 정답

**정답: B) Flash 쓰기 중 CAN 메시지 간섭 방지 → 펌웨어 무결성 보호**

### 해설

```
[OTA의 위험성]
Flash 프로그래밍 중
    + CAN 인터럽트 도착
    = 프로그래밍 중단 또는 오류
    = 펌웨어 손상 위험 ⚠️

[UDS_5의 역할]
1️⃣ Flash 전: CommunicationControl(Disable)
   → isCommunicationDisable = TRUE
   → 이후 CAN 메시지 도착 시 무시
   
2️⃣ Flash 중: ECU가 방해받지 않고 진행
   
3️⃣ Flash 후: CommunicationControl(Enable)
   → isCommunicationDisable = FALSE
   → 일반 통신 재개
```

이것이 **신뢰할 수 있는 OTA의 핵심**입니다! 🔒

---

## 문제 2 정답

### 2-1) 첫 바이트의 의미

| 메시지 | 값 | 의미 |
|--------|-----|------|
| **[TX]** | 0x03 | [00 \| 03] = FrameType:0 (SingleFrame), Length:3 |
| **[RX]** | 0x02 | [00 \| 02] = FrameType:0 (SingleFrame), Length:2 |

### 2-2) 0x28과 0x68의 관계

**ISO 14229 규칙:**
```
Response SID = Request SID + 0x40
0x28 + 0x40 = 0x68
```

### 2-3) 요청의 0x03과 0x01

| 바이트 | 값 | 의미 |
|--------|-----|------|
| **[2]** | 0x03 | SubFunc = disableRxAndTx |
| **[3]** | 0x01 | CommType = normalCommunicationMessages |

### 2-4) 요청 4바이트 vs 응답 2바이트

```
[요청] 3바이트 데이터
    SID(1) + SubFunc(1) + CommType(1) = 3 항목 필요
    
[응답] 2바이트 데이터
    SID(1) + SubFunc(1) = 2 항목만 필요
    CommType는 이미 받았으니 echo 불필요
```

---

## 문제 3 정답

### 3-1) DefaultSession에서 요청

**실패**

**에러코드: NRC_SubFunctionNotSupportedInActiveSession**

```c
else if(stBswDcmData.stDiagStatus.currentSession != DIAG_SESSION_EXTENDED)
{
    errorResult = NRC_SubFunctionNotSupportedInActiveSession;
}
```

### 3-2) 길이 2바이트 요청

**실패**

**에러코드: NRC_IncorrectMessageLengthOrInvalidFormat**

```c
if(stBswDcmData.diagDataLengthRx != 3u)  // 정확히 3만 허용!
{
    errorResult = NRC_IncorrectMessageLengthOrInvalidFormat;
}
```

### 3-3) CommType = 0xFF

**실패**

**에러코드: NRC_RequestOutRange**

```c
else if(communicationType != CommunicationType_normalCommunicationMessages)  // 0x01만 허용
{
    errorResult = NRC_RequestOutRange;
}
```

---

## 문제 4 정답

| SubFunc | 값 | 의미 | 결과: isCommunicationDisable |
|---------|-----|------|---|
| enableRxAndTx | **0x00** | **통신 활성화** | **FALSE** |
| disableRxAndTx | 0x03 | 통신 비활성화 | TRUE |

---

## 문제 5 정답

### 입력 분석

```
diagReq[0] = 0x03 (FrameType:SingleFrame, Length:3)
diagReq[1] = 0x28 (SID: CommunicationControl)
diagReq[2] = 0x03 (SubFunc: disableRxAndTx)
diagReq[3] = 0x01 (CommType: normalCommunicationMessages)
```

### 5-1) diagDataLengthRx

```c
diagDataLengthRx = (0x03 & 0x0Fu) = 0x03
```

**답: 0x03** (3바이트)

### 5-2) diagDataBufferRx 복사

```c
memcpy(&diagDataBufferRx, &diagReq[1], 3);
```

```
diagDataBufferRx[0] = 0x28 (SID)
diagDataBufferRx[1] = 0x03 (SubFunc)
diagDataBufferRx[2] = 0x01 (CommType)
```

### 5-3) SubFunc와 CommType

```c
uint8 subFunctionId = diagDataBufferRx[1] & 0x7Fu = 0x03
uint8 communicationType = diagDataBufferRx[2] = 0x01
```

```
subFunctionId = 0x03 (disableRxAndTx)
communicationType = 0x01 (normalCommunicationMessages)
```

### 5-4) isCommunicationDisable

```c
else if(subFunctionId == SFID_CommunicationControl_disableRxAndTx)  // 0x03 매칭!
{
    stBswDcmData.stDiagStatus.isCommunicationDisable = TRUE;
}
```

**답: TRUE** (통신 비활성화됨)

### 5-5) 응답 메시지 구성

```c
stBswDcmData.diagDataLengthTx = 2u;
stBswDcmData.diagDataBufferTx[0] = (FRAME_TYPE_SINGLE << 4u) | 2u;
                                  = 0x02
stBswDcmData.diagDataBufferTx[1] = SID_CommunicationControl + 0x40u;
                                  = 0x28 + 0x40 = 0x68
stBswDcmData.diagDataBufferTx[2] = subFunctionId;
                                  = 0x03 (Echo)
```

```
diagDataBufferTx[0] = 0x02
diagDataBufferTx[1] = 0x68
diagDataBufferTx[2] = 0x03
```

### 5-6) 최종 CAN TX

패딩 처리:
```c
for(uint8 i = 3u; i < TX_MSG_LEN_PHY_RES; i++)
{
    diagDataBufferTx[i] = 0xAA;  // REF_VALUE_FILLER_BYTE
}
```

```
[7DB] 02 68 03 AA AA AA AA AA
```

---

## 📊 학습 검증

**Level 1 (기본):**
- 문제 1, 2-1, 2-2 정답 → 프로토콜 기본 개념 이해 ✅

**Level 2 (논리):**
- 문제 2-3, 2-4, 3 정답 → 메시지 포맷과 검증 로직 이해 ✅

**Level 3 (코딩):**
- 문제 4, 5 정답 → C 코드와 메시지 처리 마스터 ✅

---

## 💡 핵심 복습

| 개념 | 정리 |
|------|------|
| **요청 포맷** | [프레임:0\|길이:3] [SID:0x28] [SubFunc] [CommType] |
| **응답 포맷** | [프레임:0\|길이:2] [SID+0x40:0x68] [SubFunc] |
| **검증 순서** | 길이 (3?) → 세션 (Extended?) → CommType (0x01?) |
| **SubFunction** | 0x00 (Enable, isCommunicationDisable=FALSE) / 0x03 (Disable, TRUE) |
| **필수 세션** | ExtendedSession |
| **CommType** | 0x01 (normalCommunicationMessages)만 지원 |
| **OTA 역할** | Flash 작업 중 CAN 메시지 차단 → 펌웨어 안전성 보장 |
