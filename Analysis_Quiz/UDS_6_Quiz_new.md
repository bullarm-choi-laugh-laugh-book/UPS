# UDS_6 (RoutineControl 0x31) 퀴즈

> UDS_6_Analysis.md를 읽으며 학습한 내용을 체크하세요.
> **문제 1~5번**을 모두 풀고, 뒤에서 답을 확인하세요!

---

## 📝 문제 1: RoutineControl의 OTA 검증 역할

**UDS_6이 OTA에서 왜 필요한지 설명하세요:**

다음 중 가장 정확한 답을 고르세요:

A) ECU를 빠르게 부팅하기 위해  
B) Flash 쓰기 완료 후 **실제로 펌웨어가 실행되는지 검증**하기 위해  
C) CAN 통신 속도를 높이기 위해  
D) 메모리 용량을 확인하기 위해  

---

## 📝 문제 2: RID와 Big-Endian 포맷 분석

**다음 CAN 메시지를 분석하세요:**

```
[TX] 7D3: 04 31 01 F0 00 AA AA AA  (AliveCountIncrease 요청)
[RX] 7DB: 04 71 01 F0 00 AA AA AA  (응답)
```

**질문:**

**(2-1) 첫 바이트 0x04는 무엇을 의미하나?**

**(2-2) 0x31과 0x71의 관계는?**

**(2-3) 0xF0 0x00은 어떻게 하나의 값이 되나?**
- Big/Little Endian?
- 최종 값은?

**(2-4) AliveCountDecrease를 요청하려면 마지막 2바이트를 뭘로 변경해야 하나?**

---

## 📝 문제 3: SubFunction과 RID의 관계

**UDS_6의 특징을 판단하세요:**

**(3-1) RoutineControl은 SubFunction이 0x01(Start)만 가능하다.**

그렇다면 "증가/감소"는 어떻게 구분하나?

A) SubFunction으로 구분  
B) **RID (Routine Identifier)로 구분** ← 정답  
C) CAN ID로 구분  
D) 응답 메시지로 구분  

**(3-2) RID 값 완성:**

| RID | 값 | 의미 | 결과: isAliveCountDecrease |
|-----|-----|------|---|
| RID_AliveCountIncrease | ??? | ??? | ??? |
| RID_AliveCountDecrease | 0xF001 | ??? | ??? |

**(3-3) RID 0x1234를 요청하면?**

성공? / 실패?

에러코드는?

---

## 📝 문제 4: 요청과 응답의 구조

**다음을 완성하세요:**

**요청 메시지 (4바이트 데이터):**
```
Byte 0: [Frame:0 | Length:4]
Byte 1: SID = ???
Byte 2: SubFunc = ???
Byte 3: RID Upper = 0xF0
Byte 4: RID Lower = 0x00
```

**응답 메시지 (4바이트 데이터):**
```
Byte 0: [Frame:0 | Length:4]
Byte 1: SID+0x40 = ???
Byte 2: SubFunc Echo = ???
Byte 3: RID Upper Echo = ???
Byte 4: RID Lower Echo = ???
```

---

## 📝 문제 5: 코드 흐름 추적 (Big-Endian 포함)

**AliveCountIncrease 요청:**

```
입력: isPhysical=TRUE
      diagReq[] = [0x04, 0x31, 0x01, 0xF0, 0x00, 0x55, 0x55, 0x55]
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
diagDataBufferRx[0], [1], [2], [3]의 값은?

---

```
단계 2) REtBswDcm_bswDcmMain() → routineControl() 실행
```

**질문 5-3:**
```c
uint8 subFunctionId = stBswDcmData.diagDataBufferRx[1];
uint16_t routineIdentifier = ((uint16_t)stBswDcmData.diagDataBufferRx[2] << 8) 
                            | (uint16_t)stBswDcmData.diagDataBufferRx[3];
```
각각의 값은?

**질문 5-4:**
RID 검증 후 AliveCountIncrease 루틴이 실행되면:

```c
stBswDcmData.stDiagStatus.isAliveCountDecrease = ???
```
FALSE? / TRUE?

**질문 5-5:**
생성되는 응답 메시지:

```
diagDataBufferTx[0] = ???
diagDataBufferTx[1] = ???
diagDataBufferTx[2] = ???
diagDataBufferTx[3] = ???  (RID Upper)
diagDataBufferTx[4] = ???  (RID Lower)
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

**정답: B) Flash 쓰기 완료 후 실제로 펌웨어가 실행되는지 검증하기 위해**

### 해설

```
[OTA의 최종 검증 마일]
Flash 쓰기 완료 ≠ 펌웨어 정상 실행

[UDS_6의 역할]
1️⃣ RoutineControl(RID_AliveCountIncrease)
   → AliveCounter 증가 모드 설정
   
2️⃣ (2~3초 대기)

3️⃣ ReadDataByIdentifier (다음에 배움)
   → AliveCounter 값 읽기
   → 증가했다면 = 새 펌웨어 실행 중! ✅
```

**이것이 진정한 OTA 성공 신호입니다!** 🎯

---

## 문제 2 정답

### 2-1) 첫 바이트 0x04

```
0x04 = [0000 | 0100]
       Frame:0 (SingleFrame) + Length:4 (4바이트)
```

### 2-2) 0x31과 0x71의 관계

```
Response SID = Request SID + 0x40
0x31 + 0x40 = 0x71
```

### 2-3) 0xF0 0x00 변환 (Big-Endian)

```c
routineIdentifier = (0xF0 << 8) | 0x00
                  = 0xF000
                  = RID_AliveCountIncrease
```

**Big-Endian:** 높은 바이트(0xF0)가 먼저 전송

### 2-4) AliveCountDecrease 요청

마지막 2바이트를 **0xF0 0x01**로 변경:

```
[TX] 7D3: 04 31 01 F0 01 AA AA AA
```

---

## 문제 3 정답

### 3-1) SubFunction vs RID 구분

**정답: B) RID로 구분**

```c
static uint8 subFunctionId = diagDataBufferRx[1];  // 항상 0x01 (Start)
uint16_t routineIdentifier = (diagDataBufferRx[2] << 8) | diagDataBufferRx[3];
                            // 0xF000 또는 0xF001
```

### 3-2) RID 값 완성

| RID | 값 | 의미 | 결과: isAliveCountDecrease |
|-----|-----|------|---|
| RID_AliveCountIncrease | **0xF000** | **증가 모드** | **FALSE** |
| RID_AliveCountDecrease | 0xF001 | 감소 모드 | **TRUE** |

### 3-3) RID 0x1234 요청

**실패**

**에러코드: NRC_RequestOutRange**

```c
else if (routineIdentifier == RID_AliveCountIncrease)  // 0xF000?
{ /* ... */ }
else if (routineIdentifier == RID_AliveCountDecrease)  // 0xF001?
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
Byte 0: [Frame:0 | Length:4]
Byte 1: SID = **0x31**
Byte 2: SubFunc = **0x01**
Byte 3: RID Upper = 0xF0
Byte 4: RID Lower = 0x00
```

**응답 메시지:**
```
Byte 0: [Frame:0 | Length:4]
Byte 1: SID+0x40 = **0x71**
Byte 2: SubFunc Echo = **0x01**
Byte 3: RID Upper Echo = **0xF0**
Byte 4: RID Lower Echo = **0x00**
```

---

## 문제 5 정답

### 입력 분석

```
diagReq[0] = 0x04 (FrameType:0, Length:4)
diagReq[1] = 0x31 (SID)
diagReq[2] = 0x01 (SubFunc)
diagReq[3] = 0xF0 (RID Upper)
diagReq[4] = 0x00 (RID Lower)
```

### 5-1) diagDataLengthRx

```c
diagDataLengthRx = (0x04 & 0x0Fu) = 0x04
```

**답: 0x04** (4바이트)

### 5-2) diagDataBufferRx 복사

```c
memcpy(&diagDataBufferRx, &diagReq[1], 4);
```

```
diagDataBufferRx[0] = 0x31 (SID)
diagDataBufferRx[1] = 0x01 (SubFunc)
diagDataBufferRx[2] = 0xF0 (RID Upper)
diagDataBufferRx[3] = 0x00 (RID Lower)
```

### 5-3) SubFunc와 RID

```c
uint8 subFunctionId = diagDataBufferRx[1] = 0x01
uint16_t routineIdentifier = (0xF0 << 8) | 0x00 = 0xF000
```

```
subFunctionId = 0x01 (Start)
routineIdentifier = 0xF000 (AliveCountIncrease)
```

### 5-4) isAliveCountDecrease

```c
else if (routineIdentifier == RID_AliveCountIncrease)  // 0xF000 매칭!
{
    stBswDcmData.stDiagStatus.isAliveCountDecrease = FALSE;  // 증가 모드!
}
```

**답: FALSE** (DecreaseOFF = 증가ON)

### 5-5) 응답 메시지 구성

```c
stBswDcmData.diagDataLengthTx = 4u;
stBswDcmData.diagDataBufferTx[0] = (FRAME_TYPE_SINGLE << 4u) | 4u;
                                  = 0x04
stBswDcmData.diagDataBufferTx[1] = SID_RoutineControl + 0x40u;
                                  = 0x31 + 0x40 = 0x71
stBswDcmData.diagDataBufferTx[2] = subFunctionId;
                                  = 0x01 (Echo)
stBswDcmData.diagDataBufferTx[3] = (uint8)((0xF000 & 0xFF00) >> 8u);
                                  = 0xF0 (RID Upper)
stBswDcmData.diagDataBufferTx[4] = (uint8)(0xF000 & 0xFFu);
                                  = 0x00 (RID Lower)
```

```
diagDataBufferTx[0] = 0x04
diagDataBufferTx[1] = 0x71
diagDataBufferTx[2] = 0x01
diagDataBufferTx[3] = 0xF0
diagDataBufferTx[4] = 0x00
```

### 5-6) 최종 CAN TX

패딩 처리:
```c
for(uint8 i = 5u; i < TX_MSG_LEN_PHY_RES; i++)
{
    diagDataBufferTx[i] = 0xAA;
}
```

```
[7DB] 04 71 01 F0 00 AA AA AA
```

---

## 📊 학습 검증

**Level 1 (기본):**
- 문제 1, 2-1, 2-2 정답 → RoutineControl 개념 이해 ✅

**Level 2 (논리):**
- 문제 2-3, 2-4, 3 정답 → Big-Endian과 RID 이해 ✅

**Level 3 (코딩):**
- 문제 4, 5 정답 → 2바이트 파라미터 처리 마스터 ✅

---

## 💡 핵심 복습

| 개념 | 정리 |
|------|------|
| **요청 포맷** | [프레임:0\|길이:4] [SID:0x31] [SubFunc:0x01] [RID_Hi] [RID_Lo] |
| **응답 포맷** | [프레임:0\|길이:4] [SID+0x40:0x71] [SubFunc Echo] [RID_Hi Echo] [RID_Lo Echo] |
| **검증 순서** | 길이 (4?) → SubFunc (0x01?) → RID (정의된 값?) |
| **RID 구분** | 0xF000 (증가) / 0xF001 (감소) |
| **Big-Endian** | 상위 바이트 먼저: (Upper << 8) \| Lower |
| **상태 플래그** | isAliveCountDecrease (FALSE=증가, TRUE=감소) |
| **OTA 역할** | Flash 후 **실제 펌웨어 실행 검증** (AliveCounter로) |
| **특이점** | 세션 검증 없음, SubFunc는 0x01만 지원 |
