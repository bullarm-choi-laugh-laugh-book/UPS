# UDS_4 (SecurityAccess 0x27 + Table) 분석

> **UDS_3과의 차이**: 고정된 Seed/Key가 아니라 **테이블 기반의 동적 인증**!
> Seed가 달라질 때마다 올바른 Key도 달라진다 = 진정한 보안 🔒

---

## 1️⃣ 코드 비교: UDS_3 → UDS_4

### 함수 선언은 동일

```c
#define SID_SecurityAccess					0x27u
#define SFID_SecurityAccess_RequestSeed		0x01u
#define SFID_SecurityAccess_SendKey			0x02u

static uint8 securityAccess(void);  // UDS_4도 동일
```

---

### securityAccess() 함수 - 핵심 변화

#### UDS_3 (고정값):

```c
static uint8 securityAccess(void)
{
	uint8 subFunctionId = stBswDcmData.diagDataBufferRx[1];
	
	uint16 seed = 0x1234;      // 🔴 항상 같음!
	uint16 key;
	uint8 errorResult = FALSE;

	stBswDcmData.stDiagStatus.isSecurityUnlock = FALSE;
	
	// ... RequestSeed에서는 seed = 0x1234 항상 반환
	
	// ... SendKey에서는 
	if(key == 0x5678u)  // 🔴 0x5678만 정답!
	{
		stBswDcmData.stDiagStatus.isSecurityUnlock = TRUE;
	}
}
```

#### UDS_4 (테이블 기반):

```c
static uint8 securityAccess(void)
{
	uint8 subFunctionId = stBswDcmData.diagDataBufferRx[1];
	static uint16 genKey;      // ✅ NEW: static으로 RequestSeed에서 설정!
	uint16 seed, key;
	uint8 errorResult = FALSE;

	stBswDcmData.stDiagStatus.isSecurityUnlock = FALSE;
	
	if(stBswDcmData.stDiagStatus.currentSession == DIAG_SESSION_DEFAULT)
	{
		errorResult = NRC_ServiceNotSupportedInActiveSession;
	}
	// ✅ NEW: RequestSeed 처리
	else if(subFunctionId == SFID_SecurityAccess_RequestSeed)
	{
		if(stBswDcmData.diagDataLengthRx != 2u)
		{
			errorResult = NRC_IncorrectMessageLengthOrInvalidFormat;
		}
		else
		{
			// ✅ KEY POINT: 시간에 따라 다른 Seed/Key 선택!
			if(UT_getTimeEcuAlive1ms() % 3 == 1)  // 모듈로 연산
			{
				seed = 0x1234;
				genKey = 0x4567;  // 대응하는 Key 저장!
			}
			else if(UT_getTimeEcuAlive1ms() % 3 == 2)
			{
				seed = 0x2345;
				genKey = 0x5678;
			}
			else  // (UT_getTimeEcuAlive1ms() % 3 == 0)
			{
				seed = 0x3456;
				genKey = 0x6789;
			}
			
			// 응답: [길이][SID+0x40][SubFunc][Seed_Hi][Seed_Lo]
			stBswDcmData.diagDataLengthTx = 4u;
			stBswDcmData.diagDataBufferTx[0] = (FRAME_TYPE_SINGLE << 4u) | stBswDcmData.diagDataLengthTx;
			stBswDcmData.diagDataBufferTx[1] = SID_SecurityAccess + 0x40u;   // 0x67
			stBswDcmData.diagDataBufferTx[2] = subFunctionId;                // 0x01 에코
			stBswDcmData.diagDataBufferTx[3] = (uint8)((seed & 0xFF00u) >> 8u);  // High
			stBswDcmData.diagDataBufferTx[4] = (uint8)(seed & 0xFFu);            // Low
		}
	}
	// ✅ SendKey 처리
	else if(subFunctionId == SFID_SecurityAccess_SendKey)
	{
		if(stBswDcmData.diagDataLengthRx != 4u)
		{
			errorResult = NRC_IncorrectMessageLengthOrInvalidFormat;
		}
		else
		{
			// 진단기의 Key 추출
			key = ((uint16)stBswDcmData.diagDataBufferRx[2] << 8u) 
				| stBswDcmData.diagDataBufferRx[3];
			
			// ✅ KEY POINT: RequestSeed에서 저장된 genKey와 비교!
			if(key == genKey)  // 동적으로 설정된 값!
			{
				stBswDcmData.diagDataLengthTx = 2u;
				stBswDcmData.diagDataBufferTx[0] = (FRAME_TYPE_SINGLE << 4u) | stBswDcmData.diagDataLengthTx;
				stBswDcmData.diagDataBufferTx[1] = SID_SecurityAccess + 0x40u;  // 0x67
				stBswDcmData.diagDataBufferTx[2] = subFunctionId;               // 0x02 에코
				stBswDcmData.stDiagStatus.isSecurityUnlock = TRUE;  // ✅ 인증 완료!
			}
			else  // 틀렸다
			{
				errorResult = NRC_InvalidKey;
			}
		}
	}
	else
	{
		errorResult = NRC_SubFunctionNotSupported;
	}
	return errorResult;
}
```

---

## 2️⃣ 핵심 개념: `static uint16 genKey`

### 왜 `static` 변수를 쓸까?

```c
static uint16 genKey;  // ✅ 함수 내 static
```

**특징:**
- 함수 호출이 끝나도 값이 유지됨
- RequestSeed에서 설정한 genKey 값이
- SendKey에서도 접근 가능!

**시나리오:**

```
[1] RequestSeed 호출 (시간: 1000ms)
    │
    ├─ UT_getTimeEcuAlive1ms() = 1000
    ├─ 1000 % 3 = 1 → seed = 0x1234, genKey = 0x4567
    └─ 응답: [0x04][0x67][0x01][0x12][0x34][...]
    
[2] genKey = 0x4567은 static이므로 메모리에 저장됨!

[3] SendKey 호출 (시간: 1005ms)  ← 5ms 후
    │
    ├─ UT_getTimeEcuAlive1ms() = 1005
    ├─ 1005 % 3 = 0 → seed = 0x3456 (새로 계산, 하지만 사용 안 함)
    └─ genKey = 0x4567이 여전히 유지되어 있음! ✅
    
[4] 진단기가 보낸 Key: 0x4567
    │
    └─ if(key == genKey)  → 0x4567 == 0x4567 ✅ 성공!
```

**주의:** static이 아니었다면?

```c
uint16 genKey;  // static 없음 → 함수 호출마다 초기화!
```

```
[1] RequestSeed: genKey = 0x4567 (저장됨)
[2] SendKey: genKey = 쓰레기값 or 0 (초기화됨!) ❌
```

---

## 3️⃣ 시간 기반 Seed/Key 선택 메커니즘

### `UT_getTimeEcuAlive1ms() % 3` 분석

```c
uint32 ecu_time_ms = UT_getTimeEcuAlive1ms();
```

**의미:**
- ECU가 부팅 후 경과한 시간 (밀리초)
- 1초마다 1000 증가
- 모듈로 3: 0, 1, 2 중 하나

**매핑 테이블 (강의록 P.30):**

| `time_ms % 3` | `seed` | `genKey` | 비고 |
|-------|--------|----------|------|
| `1` | `0x1234` | `0x4567` | Mode 1 |
| `2` | `0x2345` | `0x5678` | Mode 2 |
| `0` | `0x3456` | `0x6789` | Mode 3 |

**예시:**

```
부팅 후 경과 시간:
- 1ms ~ 999ms:    1 % 3 = 1 → Seed:0x1234, Key:0x4567
- 1000ms ~ 1999ms: 1000 % 3 = 1 → Seed:0x1234, Key:0x4567
- 2000ms ~ 2999ms: 2000 % 3 = 2 → Seed:0x2345, Key:0x5678
- 3000ms ~ 3999ms: 3000 % 3 = 0 → Seed:0x3456, Key:0x6789
- 4000ms ~ 4999ms: 4000 % 3 = 1 → Seed:0x1234, Key:0x4567 (반복!)
```

---

## 4️⃣ 강의록 CAN 로그 분석 (P.30)

### SecurityAccess_1 시나리오

```
[시간: 어딘가] SecurityAccess_1 START

Step 1: RequestSeed
TX [7D3] 02 27 01 55 55 55 55 55    ← "Seed 줘!"
RX [7DB] 04 67 01 12 34 AA AA AA    ← Seed = 0x1234 (Mode 1)
                       ↑  ↑
                    High Low
                    
진단기 동작: Seed(0x1234)와 테이블에서 Key(0x4567) 대응값 찾기

Step 2: SendKey
TX [7D3] 04 27 02 45 67 55 55 55    ← Key = 0x4567
              ↑  ↑
           High Low
RX [7DB] 02 67 02 AA AA AA AA AA    ← "OK! 인증 완료 ✅"

genKey = 0x4567은 RequestSeed에서 ECU 메모리에 저장됨
진단기의 Key(0x4567) == ECU의 genKey(0x4567) → 성공!
```

### SecurityAccess_2 시나리오

```
[시간: 다른 시점] SecurityAccess_2 START
                  (time_ms % 3 값이 바뀜!)

Step 1: RequestSeed
TX [7D3] 02 27 01 55 55 55 55 55    ← "Seed 줘!"
RX [7DB] 04 67 01 23 45 AA AA AA    ← Seed = 0x2345 (Mode 2!)
                       ↑  ↑              (다른 모드!)
                    High Low

genKey = 0x5678으로 업데이트됨!

Step 2: SendKey  
TX [7D3] 04 27 02 56 78 55 55 55    ← Key = 0x5678 (대응값!)
              ↑  ↑
           High Low
RX [7DB] 02 67 02 AA AA AA AA AA    ← "OK! 인증 완료 ✅"

genKey = 0x5678 == 진단기의 Key(0x5678) → 성공!
```

### SecurityAccess 3번째

```
[또 다른 시점] SecurityAccess_3 START
               (time_ms % 3이 또 바뀜!)

Step 1: RequestSeed
TX [7D3] 02 27 01 55 55 55 55 55
RX [7DB] 04 67 01 34 56 AA AA AA    ← Seed = 0x3456 (Mode 3!)

genKey = 0x6789로 업데이트됨!

Step 2: SendKey
TX [7D3] 04 27 02 67 89 55 55 55    ← Key = 0x6789 (대응값!)
RX [7DB] 02 67 02 AA AA AA AA AA    ← "OK! 인증 완료 ✅"
```

**강의록 참고:** **P.30 "Practice : Uds - SecurityAccess"**, 이미지 `30_image_0.png`, `30_image_1.png`

---

## 5️⃣ UDS_3 vs UDS_4 비교

| 항목 | UDS_3 (고정) | UDS_4 (테이블) |
|------|------------|-------------|
| **Seed 종류** | 1가지 (0x1234) | 3가지 (0x1234, 0x2345, 0x3456) |
| **Key 종류** | 1가지 (0x5678) | 3가지 (0x4567, 0x5678, 0x6789) |
| **선택 방식** | 고정값 | 시간 기반 (time_ms % 3) |
| **변수 타입** | 로컬 변수 | static 변수 (`genKey`) |
| **보안 강도** | 약함 😅 | 강함 💪 |
| **교육 용도** | 기초 개념 | 실무 패턴 |

---

## 6️⃣ 보안 관점: 왜 테이블 기반이 좋을까?

### UDS_3의 문제점 🔴

```
공격자가:
[1] Seed 스니핑: 0x1234 (항상 같음)
[2] Key 스니핑: 0x5678 (항상 같음)
[3] 한 번 기록하면 언제든지 재사용 가능!
    → " Replay Attack" 가능!! 위험!

진단기A가 한 번 성공한 CAN 로그를 복사해서
시간이 지난 뒤에 같은 깨 Seed/Key로 공격 가능
```

### UDS_4의 보안 😊

```
시간에 따라 Seed/Key가 변함:
- 시간 T1: Seed=0x1234, Key=0x4567
- 시간 T2: Seed=0x2345, Key=0x5678
- 시간 T3: Seed=0x3456, Key=0x6789
- 시간 T4: Seed=0x1234, Key=0x4567 (반복, 하지만 1000ms 후!)

공격자가 기록한 (0x1234, 0x4567)를 
나중에 재사용 시도:
→ 그 시간에 Seed가 0x1234가 아니면 실패! ❌
→ "Time-based replay protection" ✅
```

---

## 7️⃣ 코드 상세 분석

### RequestSeed에서 genKey 설정

```c
if(UT_getTimeEcuAlive1ms() % 3 == 1)
{
    seed = 0x1234;
    genKey = 0x4567;  // ← static 변수에 저장!
}
else if(UT_getTimeEcuAlive1ms() % 3 == 2)
{
    seed = 0x2345;
    genKey = 0x5678;
}
else
{
    seed = 0x3456;
    genKey = 0x6789;
}

// 응답: Seed 전송 (genKey는 ECU 메모리에만 저장)
stBswDcmData.diagDataBufferTx[3] = (uint8)((seed & 0xFF00u) >> 8u);
stBswDcmData.diagDataBufferTx[4] = (uint8)(seed & 0xFFu);
```

**핵심:**
- **Seed는 진단기에 전송** (공개)
- **genKey는 ECU 내부에만 저장** (비공개!)

```
┌─────────────────────┐
│ ECU 메모리          │         CAN 버스
│ ┌─────────────────┐ │      ┌──────────┐
│ │ genKey=0x4567  │ │──────→│ 진단기   │
│ │ (비공개!)       │ │  Seed (0x1234만
│ └─────────────────┘ │      │ 전송)
└─────────────────────┘      └──────────┘
                                  ↓
                            진단기가 테이블 
                            확인해서 Key
                            계산(0x4567)
                                  ↓
                           SendKey로 전송
```

---

### SendKey에서 genKey 검증

```c
key = ((uint16)stBswDcmData.diagDataBufferRx[2] << 8u) 
    | stBswDcmData.diagDataBufferRx[3];
    
if(key == genKey)  // RequestSeed에서 저장한 값과 비교!
{
    stBswDcmData.stDiagStatus.isSecurityUnlock = TRUE;
}
else
{
    errorResult = NRC_InvalidKey;
}
```

**흐름:**

```
[1] RequestSeed 호출
    → genKey = 0x4567 (static 메모리에 저장)
    
[2] SendKey 호출
    → key = 진단기에서 보낸 값 추출
    → if(key == genKey) ← static 메모리에서 읽기!
    → 비교 성공 → 인증 완료
```

---

## 8️⃣ 시간 기반 선택의 실무 예시

### 실제 시나리오

```
ECU 부팅 후:

시간(ms)    time%3   Seed    Key     상태
─────────────────────────────────────────
100ms      1      0x1234  0x4567  가능
500ms      2      0x2345  0x5678  가능
1000ms     1      0x1234  0x4567  가능 (다시 Mode 1)
1500ms     2      0x2345  0x5678  가능 (다시 Mode 2)
2000ms     2      0x2345  0x5678  가능
2500ms     0      0x3456  0x6789  가능
3000ms     0      0x3456  0x6789  가능
3500ms     2      0x2345  0x5678  가능
4000ms     1      0x1234  0x4567  가능 (Mode 1 반복)
```

**특징:**
- 약 3초마다 패턴 반복
- 하지만 **절대 예측 불가능** (정확한 시간 모름)
- **3가지 모드 순환** → 공격 어려움

**참고:** 강의록 **P.30 테이블**, 이미지 `30_image_0.png`

---

## 9️⃣ OTA 플로우에서의 역할

UDS_1, UDS_2, UDS_3, **UDS_4** 모두 완전히 동일한 OTA 순서!

```
1️⃣ DiagSessionControl(0x10) → Extended
   ↓
2️⃣ SecurityAccess(0x27) ← UDS_3 or UDS_4?
   ├─ RequestSeed(0x01)
   └─ SendKey(0x02)
   ↓
3️⃣ WriteDataByIdentifier(0x2E) ← 펌웨어 쓰기
   ↓
4️⃣ ECUReset(0x11)
   ↓
5️⃣ 새 펌웨어 실행 🎉
```

**차이점:**
- UDS_3: 1가지 Seed/Key만 지원 (교육용)
- UDS_4: 3가지 Seed/Key 테이블 (실무용)

---

## 🔟 요약: UDS_1 ~ UDS_4

| 단계 | 서비스 | 세션 | 핵심 개념 | 상태 변수 |
|------|--------|------|---------|----------|
| 1 | DiagSessionControl(0x10) | 양쪽 | 진단 세션 전환 | currentSession |
| 2 | ECUReset(0x11) | 양쪽 | 하드웨어 리셋 | isEcuReset |
| 3 | SecurityAccess(0x27) - 고정 | Extended | 고정 Seed/Key | isSecurityUnlock |
| **4** | **SecurityAccess(0x27) - 테이블** | **Extended** | **시간 기반 3가지 모드** | **isSecurityUnlock** |

---

## 🎓 이해 체크리스트

- [ ] `static uint16 genKey`의 역할 이해
- [ ] RequestSeed와 SendKey 사이에 genKey가 유지되는 이유 알기
- [ ] `UT_getTimeEcuAlive1ms() % 3` 계산 가능
- [ ] 3가지 Seed/Key 테이블 암기: (0x1234→0x4567), (0x2345→0x5678), (0x3456→0x6789)
- [ ] UDS_3과 UDS_4의 보안 강도 차이 이해
- [ ] CAN 로그에서 Seed/Key 값 읽기 가능

---

**강의록 전체 참고:**
- **P.28**: SecurityAccess 메시지 정의 (UDS_3과 동일)
- **P.30**: 테이블 기반 예시, 실제 CAN 로그 (SecurityAccess 3회)
- **이미지**: 30_image_0.png, 30_image_1.png (CAN 로그 캡처)
