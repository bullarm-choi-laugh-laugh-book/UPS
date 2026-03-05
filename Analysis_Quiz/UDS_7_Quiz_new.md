# UDS_7 (WriteDataByIdentifier 0x2E) 퀴즈

> UDS_7_Analysis.md를 읽으며 학습한 내용을 체크하세요.
> **문제 1~5번**을 모두 풀고, 뒤에서 답을 확인하세요!

---

## 📝 문제 1: WriteDataByIdentifier의 역할

**UDS_7이 OTA에서 하는 역할은?**

다음 중 가장 정확한 답을 고르세요:

A) Flash 속도를 높이기 위해  
B) Flash 쓰기 후 **펌웨어 버전(VariantCode)을 NVM에 영구 저장**하기 위해  
C) ECU 통신 속도를 조절하기 위해  
D) 진단기의 암호를 변경하기 위해  

---

## 📝 문제 2: 요청/응답 메시지 포맷 분석

**다음 CAN 메시지를 분석하세요:**

```
[TX] 7D3: 07 2E F1 01 [AA BB CC DD] AA  (VariantCode 쓰기 요청)
[RX] 7DB: 03 6E F1 01 AA AA AA AA AA  (응답)
```

**질문:**

**(2-1) 첫 바이트 0x07과 0x03은 각각 뭘 의미하나?**

**(2-2) 0x2E와 0x6E의 관계는?**

**(2-3) F1 01은 뭐고, AA BB CC DD는 뭐인가?**

**(2-4) 요청은 7바이트인데 응답은 3바이트인 이유는?**

---

## 📝 문제 3: 검증 단계 (3+1 구조)

**다음 시나리오를 판단하세요:**

**(3-1) DefaultSession에서 WriteDataByIdentifier 요청**

성공? / 실패?

에러코드는?

**(3-2) ExtendedSession이지만 SecurityUnlock=FALSE인 경우**

```
요청: 07 2E F1 01 AA BB CC DD
```

성공? / 실패?

에러코드는?

**(3-3) ExtendedSession + SecurityUnlock=TRUE, 하지만 DID=0x1234인 경우**

성공? / 실패?

에러코드는?

---

## 📝 문제 4: VariantCode 데이터 구조

**다음을 완성하세요:**

```
요청 메시지:
Byte 0: [Frame:0 | Length:7]
Byte 1: SID = ???
Byte 2: DID Upper = ???
Byte 3: DID Lower = ???
Byte 4: Data[0] = ???
Byte 5: Data[1] = ???
Byte 6: Data[2] = ???
Byte 7: Data[3] = ???
```

**DATA_LEN_VariantCode = ???바이트**

---

## 📝 문제 5: 코드 흐름 추적 (보안 검증 포함)

**VariantCode 쓰기 요청:**

```
입력: isPhysical=TRUE
      currentSession=EXTENDED, isSecurityUnlock=TRUE
      diagReq[] = [0x07, 0x2E, 0xF1, 0x01, 0xAA, 0xBB, 0xCC, 0xDD]
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
diagDataBufferRx[0], [1], [2], [3], [4]의 값은?

---

```
단계 2) REtBswDcm_bswDcmMain() → writeDataByIdentifier() 실행
```

**질문 5-3:**
```c
uint16 dataIdentifier = ((uint16_t)diagDataBufferRx[1] << 8) 
                      | (uint16_t)diagDataBufferRx[2];
```
dataIdentifier = ?

(Big-Endian으로 계산하세요)

**질문 5-4:**
3중 검증(세션, 보안, DID) 모두 통과하면 NVM 저장:

```c
variantCode[0] = diagDataBufferRx[3] = ???
variantCode[1] = diagDataBufferRx[4] = ???
variantCode[2] = diagDataBufferRx[5] = ???
variantCode[3] = diagDataBufferRx[6] = ???
```

**질문 5-5:**
생성되는 응답 메시지:

```
diagDataBufferTx[0] = ???
diagDataBufferTx[1] = ???
diagDataBufferTx[2] = ???  (DID Upper)
diagDataBufferTx[3] = ???  (DID Lower)
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

**정답: B) Flash 쓰기 후 펌웨어 버전(VariantCode)을 NVM에 영구 저장하기 위해**

### 해설

```
[OTA 완료 확인]
1️⃣ Flash에 새 펌웨어 설치 ✅
2️⃣ AliveCounter 증감 확인 ✅
3️⃣ VariantCode를 NVM에 저장 ← UDS_7의 역할!
   = 향후 언제든 "어떤 버전이 설치되었는지" 확인 가능

[NVM의 의미]
전원을 꺼도 데이터 유지 → 재부팅 후에도 버전 정보 남음
```

이것이 **OTA 기록 관리**의 핵심입니다! 📝

---

## 문제 2 정답

### 2-1) 첫 바이트 의미

| 메시지 | 값 | 의미 |
|--------|-----|------|
| **[TX]** | 0x07 | [00 \| 07] = FrameType:0, Length:7 |
| **[RX]** | 0x03 | [00 \| 03] = FrameType:0, Length:3 |

### 2-2) 0x2E와 0x6E의 관계

```
Response SID = Request SID + 0x40
0x2E + 0x40 = 0x6E
```

### 2-3) F1 01과 AA BB CC DD

| 항목 | 값 | 의미 |
|------|-----|------|
| **F1 01** | DID (2바이트) | 0xF101 = DID_VariantCode |
| **AA BB CC DD** | Data (4바이트) | VariantCode 값 |

따라서 요청은:
```
[SID][DID_Hi][DID_Lo][Data0][Data1][Data2][Data3]
= 1 + 1 + 1 + 1 + 1 + 1 + 1 = 7바이트
```

### 2-4) 요청 7바이트 vs 응답 3바이트

```
[요청] 데이터 전송
    SID + DID + Data(4) = 7바이트

[응답] 확인만 전송
    SID + DID(Echo) = 3바이트
    Data는 echo할 필요 없음 (이미 받았으니)
```

---

## 문제 3 정답

### 3-1) DefaultSession에서 요청

**실패**

**에러코드: NRC_ServiceNotSupportedInActiveSession**

```c
if(stBswDcmData.stDiagStatus.currentSession != DIAG_SESSION_EXTENDED)
{
    errorResult = NRC_ServiceNotSupportedInActiveSession;
}
```

### 3-2) Unlock=FALSE

**실패**

**에러코드: NRC_SecurityAccessDenied**

```c
else if(stBswDcmData.stDiagStatus.isSecurityUnlock == FALSE)
{
    errorResult = NRC_SecurityAccessDenied;
}
```

### 3-3) DID=0x1234

**실패**

**에러코드: NRC_RequestOutRange**

```c
else if(dataIdentifier == DID_VariantCode)  // 0xF101만 지원
{ /* ... */ }
else
{
    errorResult = NRC_RequestOutRange;  // ← 0x1234는 미지원!
}
```

---

## 문제 4 정답

**요청 메시지:**
```
Byte 0: [Frame:0 | Length:7]
Byte 1: SID = **0x2E**
Byte 2: DID Upper = **0xF1**
Byte 3: DID Lower = **0x01**
Byte 4: Data[0] = AA
Byte 5: Data[1] = BB
Byte 6: Data[2] = CC
Byte 7: Data[3] = DD
```

**DATA_LEN_VariantCode = **4**바이트**

---

## 문제 5 정답

### 입력 분석

```
diagReq[0] = 0x07 (FrameType:0, Length:7)
diagReq[1] = 0x2E (SID)
diagReq[2] = 0xF1 (DID Upper)
diagReq[3] = 0x01 (DID Lower)
diagReq[4] = 0xAA (Data[0])
diagReq[5] = 0xBB (Data[1])
diagReq[6] = 0xCC (Data[2])
diagReq[7] = 0xDD (Data[3])
```

### 5-1) diagDataLengthRx

```c
diagDataLengthRx = (0x07 & 0x0Fu) = 0x07
```

**답: 0x07** (7바이트)

### 5-2) diagDataBufferRx 복사

```c
memcpy(&diagDataBufferRx, &diagReq[1], 7);
```

```
diagDataBufferRx[0] = 0x2E (SID)
diagDataBufferRx[1] = 0xF1 (DID Upper)
diagDataBufferRx[2] = 0x01 (DID Lower)
diagDataBufferRx[3] = 0xAA (Data[0])
diagDataBufferRx[4] = 0xBB (Data[1])
```

### 5-3) dataIdentifier (Big-Endian)

```c
dataIdentifier = (0xF1 << 8) | 0x01
               = 0xF100 | 0x01
               = 0xF101 (DID_VariantCode)
```

**답: 0xF101**

### 5-4) variantCode 배열

```c
variantCode[0] = diagDataBufferRx[3] = **0xAA**
variantCode[1] = diagDataBufferRx[4] = **0xBB**
variantCode[2] = diagDataBufferRx[5] = **0xCC**
variantCode[3] = diagDataBufferRx[6] = **0xDD**
```

이후 NVM에 저장:
```c
Rte_Call_BswDcm_rEcuAbsNvm_writeNvmData(DATA_ID_VariantCode, variantCode);
```

### 5-5) 응답 메시지 구성

```c
stBswDcmData.diagDataLengthTx = 3u;
stBswDcmData.diagDataBufferTx[0] = (FRAME_TYPE_SINGLE << 4u) | 3u;
                                  = 0x03
stBswDcmData.diagDataBufferTx[1] = SID_WriteDataByIdentifier + 0x40u;
                                  = 0x2E + 0x40 = 0x6E
stBswDcmData.diagDataBufferTx[2] = (uint8)((0xF101 & 0xFF00) >> 8u);
                                  = 0xF1 (DID Upper)
stBswDcmData.diagDataBufferTx[3] = (uint8)(0xF101 & 0xFFu);
                                  = 0x01 (DID Lower)
```

```
diagDataBufferTx[0] = 0x03
diagDataBufferTx[1] = 0x6E
diagDataBufferTx[2] = 0xF1
diagDataBufferTx[3] = 0x01
```

### 5-6) 최종 CAN TX

패딩 처리:
```c
for(uint8 i = 4u; i < TX_MSG_LEN_PHY_RES; i++)
{
    diagDataBufferTx[i] = 0xAA;
}
```

```
[7DB] 03 6E F1 01 AA AA AA AA
```

---

## 📊 학습 검증

**Level 1 (기본):**
- 문제 1, 2-1, 2-2 정답 → WriteDataByIdentifier 개념 이해 ✅

**Level 2 (논리):**
- 문제 2-3, 2-4, 3 정답 → 3중 검증 and NVM 개념 이해 ✅

**Level 3 (코딩):**
- 문제 4, 5 정답 → NVM 저장 흐름 마스터 ✅

---

## 💡 핵심 복습

| 개념 | 정리 |
|------|------|
| **요청 포맷** | [프레임:0\|길이:7] [SID:0x2E] [DID_Hi] [DID_Lo] [4바이트 Data] |
| **응답 포맷** | [프레임:0\|길이:3] [SID+0x40:0x6E] [DID_Hi Echo] [DID_Lo Echo] |
| **검증 순서** | 세션(Extended) → 보안(Unlock) → DID(0xF101?) → 길이(7?) |
| **DID** | 0xF101 = VariantCode (펌웨어 버전) |
| **Data 크기** | 4바이트 (DATA_LEN_VariantCode) |
| **NVM 저장** | Rte_Call_BswDcm_rEcuAbsNvm_writeNvmData() 호출 |
| **영구성** | 전원 재인가 후에도 데이터 유지 |
| **Big-Endian** | DID도 2바이트 Big-Endian |
| **OTA 역할** | Flash 완료 후 **버전 정보 영구 기록** |
