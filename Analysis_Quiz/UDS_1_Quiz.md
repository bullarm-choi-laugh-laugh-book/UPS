# UDS_1 (DiagnosticSessionControl 0x10) 퀴즈

> UDS_1_Analysis.md를 읽으며 학습한 내용을 체크하세요.
> 먼저 **문제 1~5번**을 모두 풀고, 답은 뒤에서 확인하세요!

---

## 📝 문제 1: DiagnosticSessionControl의 역할

**DiagnosticSessionControl(0x10)이 왜 "첫 번째" 서비스인가?**

다음 중 가장 적절한 답을 고르세요:

A) 서비스 ID가 가장 작아서  
B) 다른 서비스들(SecurityAccess, WriteDataByIdentifier 등)이 이후에만 실행 가능하기 때문  
C) 가장 자주 사용되니까  
D) ECU가 부팅될 때 가장 먼저 실행되니까  

---

## 📝 문제 2: CAN 메시지 프로토콜 분석

**다음 CAN 메시지를 분석하세요:**

```
[TX] 7D3: 02 10 03 55 55 55 55 55
[RX] 7DB: 06 50 03 00 32 13 88 AA
```

**질문:**

**(2-1) 첫 번째 바이트 0x02와 0x10은 각각 뭘 의미하나?**

**(2-2) 왜 응답이 0x50인가? (원래 0x10에서 어떻게 변했나?)**

**(2-3) 응답에서 0x00 0x32 0x13 0x88은 무엇을 나타내나?**

**(2-4) 응답 메시지 마지막의 0xAA는 왜 붙어있나?**

---

## 📝 문제 3: 코드 아키텍처 이해

**UDS_1 코드에서 두 함수가 분리되어 있습니다:**

```
1) REoiBswDcm_pDcmCmd_processDiagRequest()  ← 요청 수신
        ↓
   stBswDcmData.isServiceRequest = TRUE
        ↓
2) REtBswDcm_bswDcmMain()  ← 메인 루프
        ↓
   if(isServiceRequest == TRUE) { ... }
```

**질문: 왜 이렇게 두 함수로 나뉘어있나? 한 함수에서 다 처리하면 안 되나?**

다음 중 가장 적절한 이유를 고르세요:

A) 코드를 깔끔하게 정리하기 위해  
B) 함수 1은 CAN 인터럽트 핸들러 (빨리 끝나야 함), 함수 2는 메인 루프 (시간 여유 있음)  
C) ISO 표준에서 두 함수를 쓰게 규정했으니까  
D) 메모리 절약 때문에  

---

## 📝 문제 4: 세션 제약 이해

**강의록 P.6의 "Table 23"을 참고하여 다음을 판단하세요:**

**(4-1) Default 세션에서 0x27(SecurityAccess) 서비스 요청**

성공? / 실패?

만약 실패라면 어떤 에러코드가 반환될까?
- NRC_ServiceNotSupported (0x11)
- NRC_ServiceNotSupportedInActiveSession (0x7F)
- NRC_SubFunctionNotSupported (0x12)

**(4-2) Default 세션에서 0x10(DiagSessionControl) 서비스 요청**

성공? / 실패?

**(4-3) Extended 세션에서 0x2E(WriteDataByIdentifier) 서비스 요청**

성공? / 실패?

**(4-4) "ServiceNotSupported"와 "ServiceNotSupportedInActiveSession"의 차이점을 설명하세요.**

---

## 📝 문제 5: 코드 흐름 추적

**다음 시나리오를 추적하세요.**

진단기가 Extended 세션으로 전환하라고 요청합니다:

```
입력:  isPhysical=TRUE
       diagReq[] = [0x02, 0x10, 0x03, 0x55, 0x55, 0x55, 0x55, 0x55]
             (이 배열은 CAN에서 받은 원본 데이터)

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
diagDataBufferRx에 복사되는 데이터는? (처음 2바이트만 쓰세요)

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
```
subFunctionId의 값은?

---

```
단계 3) diagnosticSessionControl() 함수
```

**질문 5-6:**
이 함수가 성공했을 때 생성되는 응답 메시지를 채워보세요:

```
diagDataBufferTx[0] = ???  (프레임 타입|길이)
diagDataBufferTx[1] = ???  (서비스 응답 코드)
diagDataBufferTx[2] = ???  (서브함수 에코)
diagDataBufferTx[3-4] = 0x00 0x32  (P2 타이밍 = 50ms)
diagDataBufferTx[5-6] = 0x13 0x88  (P2* 타이밍 = 5000ms)
```

**질문 5-7:**
최종 CAN TX 메시지 (패딩 포함) 예상 값:

```
[7DB] ?? ?? ?? ?? ?? ?? ?? ??
```

---

---

---

# 🎯 답 & 해설

## 문제 1 정답: DiagnosticSessionControl의 역할

**정답: B) 다른 서비스들(SecurityAccess, WriteDataByIdentifier 등)이 이후에만 실행 가능하기 때문**

### 해설

강의록 **P.6 "Table 23 - Services allowed during default and non-default diagnostic session"**을 보면:

```
┌──────────────────────────────┬─────────────┬──────────────────┐
│ Service                      │ DefaultSess │ ExtendedSess     │
├──────────────────────────────┼─────────────┼──────────────────┤
│ DiagnosticSessionControl 0x10│      ✅     │        ✅        │ ← 항상 가능
│ SecurityAccess 0x27          │      ❌     │        ✅        │ ← Extended만!
│ WriteDataByIdentifier 0x2E   │      ❌     │        ✅        │ ← Extended만!
│ ReadDataByIdentifier 0x22    │      ✅     │        ✅        │
└──────────────────────────────┴─────────────┴──────────────────┘
```

- **SecurityAccess(0x27)**와 **WriteDataByIdentifier(0x2E)**는 **Extended 세션 전용**
- 따라서 이 서비스들을 사용하려면 **먼저** 0x10으로 세션을 전환해야 함
- 즉, 0x10은 모든 진단 서비스의 **게이트웨이** 역할!

---

## 문제 2 정답: CAN 메시지 프로토콜 분석

### 2-1 첫 번째 바이트의 의미

**답:**
- `0x02` = Single Frame, 길이 2바이트
  - 상위 4비트: 0 = SINGLE 프레임 타입
  - 하위 4비트: 2 = 이 메시지 데이터 길이
  
- `0x10` = 서비스 ID (DiagnosticSessionControl)

**참고:** 강의록 **P.10-11 "Uds : Sessioncontrol"**, 이미지 `10_image_0.png`

---

### 2-2 왜 응답이 0x50인가?

**답:** `0x10 + 0x40 = 0x50`

ISO 14229 표준 규칙:
- 긍정 응답 = 서비스 ID + 0x40
- 부정 응답 = 0x7F + 원래 SID

```
요청:  0x10 (DiagnosticSessionControl)
응답:  0x50 (0x10 + 0x40 = 긍정 응답)
```

**참고:** 강의록 **P.24 "Table 35 - Positive Response Message Definition"**: ECUReset Response SID = 0x51 (0x11 + 0x40)

---

### 2-3 응답의 0x00 0x32 0x13 0x88은?

**답:** P2/P2* 타이밍 파라미터

```
0x00 0x32 = P2 타이밍 (50ms)
  → ECU가 정상 진단 요청에 응답해야 하는 최대 시간
  → 응답이 없으면 진단기는 타임아웃 처리

0x13 0x88 = P2* 타이밍 (5000ms)
  → ECU가 "지금 바쁘니까 5초만 기다려" 신호를 보낼 때의 대기 시간
  → 플래싱, 프로그래밍 중일 때 사용
```

계산:
- 0x0032 (16진) = 50 (10진) ms
- 0x1388 (16진) = 5000 (10진) ms ÷ 10 (코드에서 /10으로 나눔)

**참고:** 강의록 **P.11 "Table 28 - sessionParameterRecord definition"**, **P.11 "Table 29 - sessionParameterRecord content definition"**

---

### 2-4 응답 메시지 마지막의 0xAA는?

**답:** **Filler Byte (패딩 바이트)**

CAN 메시지 규칙:
- CAN 데이터에 길이가 8바이트 미만이면, 나머지를 **특정 바이트로 채워야 함**
- 요청: Filler Byte = **0x55**
- 응답: Filler Byte = **0xAA**

```
응답 메시지:  06 50 03 00 32 13 88  ← 7바이트
                                  AA  ← 8번째 바이트 (패딩)
```

**참고:** 강의록 **P.11 "Sub-function parameter"**: "Response 메시지는 Filler Byte AA hex로 채워진다"

---

## 문제 3 정답: 코드 아키텍처 이해

**정답: B) 함수 1은 CAN 인터럽트 핸들러 (빨리 끝나야 함), 함수 2는 메인 루프 (시간 여유 있음)**

### 해설

```
REoiBswDcm_pDcmCmd_processDiagRequest()
├─ 언제: CAN RX 인터럽트 발생했을 때
├─ 역할: 데이터 복사 + 플래그 설정만 (매우 빠르게)
├─ 이유: 인터럽트는 오래 실행되면 안 됨 (CAN 버스 블로킹)
└─ 시간: ~100 마이크로초

→ 플래그 설정

REtBswDcm_bswDcmMain()
├─ 언제: 메인 루프 (10ms마다)
├─ 역할: 실제 처리 + 응답 생성 + CAN 전송
├─ 이유: 여유 있는 환경에서 복잡한 로직 처리
└─ 시간: ~1-5ms (충분한 여유)
```

**장점:**
- 인터럽트를 금방 반환 → CAN 버스 안정성
- 복잡한 로직은 메인 루프에서 → 메모리/성능 안정
- 백그라운드 처리 가능

**참고:** 강의록 **P.16 "Rte_BswDcm.c" 코드**, 이미지 `16_image_0.png`

---

## 문제 4 정답: 세션 제약 이해

### 4-1 Default 세션에서 0x27(SecurityAccess) 요청

**답: 실패**

**에러코드:** `NRC_ServiceNotSupportedInActiveSession (0x7F)`

이유: Table 23에서 SecurityAccess는 Extended 세션 전용 → Default에서는 불가

---

### 4-2 Default 세션에서 0x10(DiagSessionControl) 요청

**답: 성공 ✅**

이유: Table 23에서 DiagSessionControl은 양쪽 세션에서 모두 가능

---

### 4-3 Extended 세션에서 0x2E(WriteDataByIdentifier) 요청

**답: 성공 ✅**

이유: Table 23에서 WriteDataByIdentifier는 Extended 세션에서 가능

---

### 4-4 두 에러 코드의 차이

**NRC_ServiceNotSupported (0x11):**
- 어떤 세션에서든 ECU가 **지원하지 않는 서비스**
- 예: 0x99 (존재하지 않는 서비스)

**NRC_ServiceNotSupportedInActiveSession (0x7F):**
- **서비스는 존재하지만** 현재 세션에서만 불가
- 예: Default 세션에서 0x27(SecurityAccess) 요청
- 같은 서비스를 Extended 세션에서 요청하면 성공

**참고:** 강의록 **P.21 "Figure 6 - General server response behaviour for request messages with sub-function parameter"**, **P.22 "#define NRC_SubFunctionNotSupportedInActiveSession 0x7Eu"**

---

## 문제 5 정답: 코드 흐름 추적

입력 분석:
```
diagReq[] = [0x02, 0x10, 0x03, 0x55, 0x55, 0x55, 0x55, 0x55]
            └─0: 프레임  ─1: SID  ─2: SubFunc  ─3~7: 패딩
```

---

### 5-1 frameType의 값

**답: 0**

```c
frameType = diagReq[0] >> 4u
         = 0x02 >> 4
         = 0000 0010 >> 4
         = 0000 0000
         = 0  ← SINGLE 프레임
```

---

### 5-2 diagDataLengthRx의 값

**답: 2**

```c
diagDataLengthRx = diagReq[0] & 0x0Fu
                = 0x02 & 0x0F
                = 0000 0010 & 0000 1111
                = 0000 0010
                = 2  ← 데이터 길이 2바이트
```

---

### 5-3 diagDataBufferRx에 복사되는 데이터

**답: 0x10, 0x03**

```c
memcpy(diagDataBufferRx, &diagReq[1], 2);
//                       ↑ diagReq[1]부터 시작
//                       = &[0x10, 0x03, 0x55, ...]
```

따라서 `diagDataBufferRx[0] = 0x10`, `diagDataBufferRx[1] = 0x03`

---

### 5-4 serviceId의 값

**답: 0x10**

```c
serviceId = stBswDcmData.diagDataBufferRx[0]
          = 0x10  ← DiagnosticSessionControl
```

---

### 5-5 subFunctionId의 값

**답: 0x03**

```c
subFunctionId = stBswDcmData.diagDataBufferRx[1] & 0x7Fu
             = 0x03 & 0x7F
             = 0x03  ← ExtendedSession
```

(비트 7을 마스킹하는 이유: ISO 표준에서 비트 7은 예약됨)

---

### 5-6 응답 메시지 구성

**답:**

```c
diagDataBufferTx[0] = (0 << 4u) | 6 = 0x06
  // SINGLE(0) | 길이6

diagDataBufferTx[1] = 0x10 + 0x40 = 0x50
  // SID + 0x40 = 긍정 응답

diagDataBufferTx[2] = 0x03
  // SubFunc 에코
```

**참고:** 강의록 **P.17 코드**, 이미지 `17_image_0.png`

---

### 5-7 최종 CAN TX 메시지

**답:**

```
[7DB] 06 50 03 00 32 13 88 AA
      └─┬─┘ └──────────────────┘ └─┘
        │         응답              패딩
        └─ 프레임|길이(6)
```

전체 해석:
- `06` = SINGLE 프레임, 길이 6
- `50` = 긍정 응답 (0x10 + 0x40)
- `03` = Extended 세션 에코
- `00 32` = P2 (50ms)
- `13 88` = P2* (5000ms)
- `AA` = Filler Byte

**참고:** 강의록 **P.18, 이미지 `18_image_1.png`**: 실제 로그에서 동일한 패턴

---

## 📊 채점 기준

| 문제 | 만점 | 난이도 | 핵심 개념 |
|------|------|--------|---------|
| 1 | 1점 | ⭐ 쉬움 | 게이트웨이 개념 |
| 2 | 4점 | ⭐⭐ 중간 | ISO 15765 프로토콜 |
| 3 | 1점 | ⭐⭐⭐ 어려움 | 아키텍처 이해 |
| 4 | 4점 | ⭐⭐ 중간 | Table 23 세션 제약 |
| 5 | 7점 | ⭐⭐⭐⭐ 매우 어려움 | 전체 흐름 추적 |
| **합계** | **17점** |  |  |

---

## 🎓 결과 해석

```
15-17점: 완벽! UDS_2/3로 진행해도 문제 없음 🚀
12-14점: 좋음! 좀 더 봐도 되지만, 헷갈리는 부분 재확인 추천 📖
9-11점: 보통! UDS_1의 "2️⃣ 코드 비교" 섹션 한 번 더 읽기 권장 🔄
6-8점: 아직도! P.16-17 코드 흐름을 천천히 추적해보기 🐢
0-5점: 다시 한 번! UDS_1_Analysis.md 처음부터 읽기 📚
```

---

**점수 나왔으면 말씀해주세요! 그리고 어려웠던 부분이 있으면 설명해드릴 수 있습니다.** 😊
