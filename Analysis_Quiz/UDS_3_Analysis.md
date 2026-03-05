# UDS_3 (SecurityAccess 0x27) 분석

> **OTA 보안 핵심**: 누구나 펌웨어를 업데이트할 수 있나? 
> → **아니다!** SecurityAccess로 **인증된 진단기**만 가능

---

## 1️⃣ 코드 비교: UDS_2 → UDS_3

### 추가된 Define

```c
// ✅ NEW: UDS_3에서 추가됨
#define SID_SecurityAccess					0x27u
#define SFID_SecurityAccess_RequestSeed		0x01u
#define SFID_SecurityAccess_SendKey			0x02u
```

**의미:**
- `0x27`: SecurityAccess 서비스 ID
- `0x01`: Seed 요청 (진단기 → ECU)
- `0x02`: Key 전송 (진단기 → ECU, 응답)

---

### 함수 선언 추가

```c
// UDS_2에서는 없었음
static uint8 securityAccess(void);  // ✅ NEW
```

---

### REtBswDcm_bswDcmMain() 확장

```c
void REtBswDcm_bswDcmMain(void)
{
	uint8 serviceId;
	uint8 errorResult = FALSE;
	if(stBswDcmData.isServiceRequest == TRUE)
	{
		stBswDcmData.isServiceRequest = FALSE;
		serviceId = stBswDcmData.diagDataBufferRx[0];
		if(serviceId == SID_DiagSessionControl)
		{
			errorResult = diagnosticSessionControl();
		}
		else if(serviceId == SID_EcuReset)
		{
			errorResult = ecuReset();
		}
		else if(serviceId == SID_SecurityAccess)  // ✅ NEW: UDS_3에서 추가
		{
			errorResult = securityAccess();
		}
		else
		{
			errorResult = NRC_ServiceNotSupported;
		}
		// ... (나머지는 UDS_2와 동일)
	}
}
```

**변화:**
- UDS_2: DiagSessionControl, EcuReset 두 서비스만
- UDS_3: **SecurityAccess 서비스 추가** (3번째)

---

### securityAccess() 함수 (완전히 NEW)

```c
static uint8 securityAccess(void)
{
	uint8 subFunctionId = stBswDcmData.diagDataBufferRx[1];  // SubFunc 추출
	uint16 seed = 0x1234;                                    // 😅 Hardcoded!
	uint16 key;
	uint8 errorResult = FALSE;
	stBswDcmData.stDiagStatus.isSecurityUnlock = FALSE;      // 기본값: 잠금

	// ✅ 1단계: 세션 체크
	if(stBswDcmData.stDiagStatus.currentSession == DIAG_SESSION_DEFAULT)
	{
		errorResult = NRC_ServiceNotSupportedInActiveSession;  // ← Extended만 가능!
	}
	// ✅ 2단계: RequestSeed (진단기가 Seed 요청)
	else if(subFunctionId == SFID_SecurityAccess_RequestSeed)
	{
		if(stBswDcmData.diagDataLengthRx != 2u)
		{
			errorResult = NRC_IncorrectMessageLengthOrInvalidFormat;
		}
		else
		{
			// 응답: [길이][SID+0x40][SubFunc][Seed_Hi][Seed_Lo]
			stBswDcmData.diagDataLengthTx = 4u;
			stBswDcmData.diagDataBufferTx[0] = (FRAME_TYPE_SINGLE << 4u) | stBswDcmData.diagDataLengthTx;
			stBswDcmData.diagDataBufferTx[1] = SID_SecurityAccess + 0x40u;  // 0x67
			stBswDcmData.diagDataBufferTx[2] = subFunctionId;               // 0x01 에코
			stBswDcmData.diagDataBufferTx[3] = (uint8)((seed & 0xFF00u) >> 8u);  // High
			stBswDcmData.diagDataBufferTx[4] = (uint8)(seed & 0xFFu);            // Low
		}
	}
	// ✅ 3단계: SendKey (진단기가 계산한 Key 전송)
	else if(subFunctionId == SFID_SecurityAccess_SendKey)
	{
		if(stBswDcmData.diagDataLengthRx != 4u)
		{
			errorResult = NRC_IncorrectMessageLengthOrInvalidFormat;
		}
		else
		{
			// 진단기의 Key 추출: [SID][SubFunc][Key_Hi][Key_Lo]
			key = ((uint16)stBswDcmData.diagDataBufferRx[2] << 8u) | stBswDcmData.diagDataBufferRx[3];
			
			// 🎯 핵심: 하드코딩된 Key와 비교!
			if(key == 0x5678u)  // 정답!
			{
				// 응답: [길이][SID+0x40][SubFunc]
				stBswDcmData.diagDataLengthTx = 2u;
				stBswDcmData.diagDataBufferTx[0] = (FRAME_TYPE_SINGLE << 4u) | stBswDcmData.diagDataLengthTx;
				stBswDcmData.diagDataBufferTx[1] = SID_SecurityAccess + 0x40u;  // 0x67
				stBswDcmData.diagDataBufferTx[2] = subFunctionId;               // 0x02 에코
				
				stBswDcmData.stDiagStatus.isSecurityUnlock = TRUE;  // ✅ 인증 완료!
			}
			else  // 틀렸다!
			{
				errorResult = NRC_InvalidKey;  // ← 진단기가 알 수 있음
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

## 2️⃣ 메시지 프로토콜: 2단계 인증

### 강의록 P.28-30 메시지 정의

**RequestSeed (SubFunc=0x01):**
```
진단기 → ECU (요청):
[0x02] [0x27] [0x01] [0x55] [0x55] [0x55] [0x55] [0x55]
 길이   SID    SubFunc  Padding (Filler)

ECU → 진단기 (응답):
[0x04] [0x67] [0x01] [Seed_Hi] [Seed_Lo] [0xAA] [0xAA] [0xAA]
 길이   SID+40  에코    0x12      0x34      Filler
```

**SendKey (SubFunc=0x02):**
```
진단기 → ECU (요청):
[0x04] [0x27] [0x02] [Key_Hi] [Key_Lo] [0x55] [0x55] [0x55]
 길이   SID    SubFunc  0x56     0x78    Padding

ECU → 진단기 (응답):
[0x02] [0x67] [0x02] [0xAA] [0xAA] [0xAA] [0xAA] [0xAA]
 길이   SID+40  에코    Filler (의미 없음)

또는 실패:
[0x03] [0x7F] [0x27] [0x35] [Filler...]
 길이   NegResp SID    NRC_InvalidKey
```

**참고:** 강의록 **P.28 "Table 40/41 - Request message definition"**, 이미지 `28_image_0.png`

---

## 3️⃣ 강의록 CAN 로그 분석

### 실제 성공 시나리오 (P.30 예시)

```
[12.100] SecurityAccess_1 START

Step 1: RequestSeed
TX [7D3] 02 27 01 55 55 55 55 55      ← 진단기: "Seed 줘!"
RX [7DB] 04 67 01 12 34 AA AA AA      ← ECU: "Seed는 0x1234"

Step 2: SendKey
TX [7D3] 04 27 02 56 78 55 55 55      ← 진단기: "Key는 0x5678"
RX [7DB] 02 67 02 AA AA AA AA AA      ← ECU: "OK! 인증됨"

[12.133] SecurityAccess_1 PASS ✅
```

### 실패 시나리오 (잘못된 Key)

```
Step 2: SendKey (Wrong)
TX [7D3] 04 27 02 FF FF 55 55 55      ← 진단기: "Key는 0xFFFF"
RX [7DB] 03 7F 27 35 AA AA AA AA      ← ECU: "틀렸어! (0x35=InvalidKey)"

[Error] SecurityAccess 실패 ❌
```

**참고:** 강의록 **P.30 "SecurityAccess_1/2"**, 이미지 `30_image_0.png`, `30_image_1.png`: 실제 CAN 로그 캡처

---

## 4️⃣ 핵심 개념: Seed/Key 인증 vs 고정 암호

### 현재 UDS_3는 "Fake Security" 😅

```
문제점:
  seed = 0x1234 (항상 같음!)
  key = 0x5678  (항상 같음!)
  
즉, "Seed 기반 동적 인증"이 아니라
"고정된 Password" ← 실제 제품에서는 절대 안 됨!
```

### 제대로 된 Seed/Key 인증 (실제 산업 제품)

[![.](https://mermaid.ink/img/pako:eNpdUMFuwjAM/RWLT1wgbQOb0AaCDkoP0w7sVhKa1kJEEpKkVfn7HFjGRFCf_fz8bO58RaUoJF4EQ9CFISApZSbVsGr8SnK-h-QlQ0YuSONMgBhc0vhfRVn3JUeVoxnP6b0pBGQQGfH5pBuVlr9VPi0r18mj4vEMSKw-LeRCvCGKsdxj5g-5G5SH9IDJmf6NlFm8gLjg_hABPm6WEwWcuBEFXKccGePSGdNiDM5sIpnP5b_B4hM8b8mJZwZGUsmkP4GfGAAv8?type=png)](https://mermaid.live/edit#pako:eNpdUMFuwjAM_RWLT1wgbQOb0AaCDkoP0w7sVhKa1kJEEpKkVfn7HFjGRFCf_fz8bO58RaUoJF4EQ9CFISApZSbVsGr8SnK-h-QlQ0YuSONMgBhc0vhfRVn3JUeVoxnP6b0pBGQQGfH5pBuVlr9VPi0r18mj4vEMSKw-LeRCvCGKsdxj5g-5G5SH9IDJmf6NlFm8gLjg_hABPm6WEwWcuBEFXKccGePSGdNiDM5sIpnP5b_B4hM8b8mJZwZGUsmkP4GfGAAv8?type=png)

```
실제 Seed/Key 메커니즘:
[1] 진단기: "Seed를 주세요"
[2] ECU: Seed를 **무작위**로 생성 → 0xABCD (이번)
[3] 진단기: Seed(0xABCD)를 고정 암호와 XOR
         → Key = Seed ⊕ MasterPassword
         → Key = 0xABCD ⊕ 0x5678 = 0xF5B5
[4] 진단기: "Key는 0xF5B5" 전송
[5] ECU: 받은 Key를 동일한 방식으로 검증
         → Key ⊕ MasterPassword == Seed인가?
         → 0xF5B5 ⊕ 0x5678 == 0xABCD인가? ✅
[6] ECU: "인증 OK!"

다음 부팅 때 Seed가 바뀜! (보안 강화)
```

### UDS_3는 교육용이지, 실제로는 이렇게 안 함

---

## 5️⃣ OTA 플로우에서의 역할

### 완전한 OTA 시퀀스

```
1️⃣ Default 세션
   ↓
2️⃣ DiagSessionControl(0x10) → Extended로 전환
   ↓
3️⃣ ECUReset(0x11) [Optional] ← "이전 상태 초기화"
   ↓
4️⃣ SecurityAccess(0x27) ← ★ KEY POINT
   ├─ Step A: RequestSeed(0x01) → Seed 획득
   ├─ Step B: 진단기에서 Key 계산
   └─ Step C: SendKey(0x02) → 인증 완료 ✅
   ↓
5️⃣ WriteDataByIdentifier(0x2E) ← 이제 가능!
   └─ 새 펌웨어를 Flash/NVM에 쓰기
   ↓
6️⃣ ECUReset(0x11) ← 새 펌웨어 적용 강제
   ↓
7️⃣ 부팅 시작
   └─ Bootloader → 새 펌웨어 로드 & 실행 🎉
```

**보안 계층:**
```
진단기
  ↓
[Layer 1] DiagSessionControl(0x10)
  목적: 어떤 세션인지 알림
  보안: None (공개 정보)
  ↓
[Layer 2] SecurityAccess(0x27) ← ★ 1차 보안!
  목적: "너는 누구니? 인증해봐"
  보안: Seed/Key 기반 인증
  ↓
[Layer 3] WriteData(0x2E) ← 2차 보안
  목적: 인증된 진단기만 데이터 수정 가능
  보안: 이전 SecurityAccess 플래그 체크
```

**참고:** 강의록 **P.5-6 "Table 23 - Session Levels"**: 각 서비스의 세션별 제약 조건

---

## 6️⃣ 주요 코드 상세 분석

### Key 추출 및 검증 부분

```c
key = ((uint16)stBswDcmData.diagDataBufferRx[2] << 8u) | stBswDcmData.diagDataBufferRx[3];
//    High Byte left shift    |    Low Byte bitwise OR
//    
//    예: diagDataBufferRx = [0x27, 0x02, 0x56, 0x78, ...]
//                                         (Key_Hi) (Key_Lo)
//                                         
//    [0x56] << 8 = 0x5600
//    [0x78]       = 0x0078
//    OR 결과      = 0x5678 ✅
```

**16비트 수 조립:**
```
High Byte → Low Byte 순서
0x56 78 (CAN 데이터)
  ↓
0x5678 (16비트 정수)

역으로 분해:
seed = 0x1234
(seed & 0xFF00u) >> 8u = 0x12
(seed & 0xFFu)         = 0x34
→ 응답: [...][0x12][0x34]
```

**고정값 비교:**
```c
if(key == 0x5678u)  // ← 항상 같은 값
{
    stBswDcmData.stDiagStatus.isSecurityUnlock = TRUE;
    // ← 다음 서비스 (WriteData)가 이 플래그 확인
}
else
{
    errorResult = NRC_InvalidKey;  // 0x35
    // ← 진단기가 NRC 수신 → "인증 실패!"
}
```

---

## 7️⃣ ST_DiagStatus 구조체 변화

### UDS_2까지

```c
typedef struct
{
    uint8 currentSession;      // 세션 상태: DEFAULT(0) or EXTENDED(1)
    uint8 isEcuReset;          // 리셋 플래그
} ST_DiagStatus;
```

### UDS_3에서 추가

```c
typedef struct
{
    uint8 currentSession;      // 세션 상태
    uint8 isEcuReset;          // 리셋 플래그
    uint8 isSecurityUnlock;    // ✅ NEW: 인증 플래그
} ST_DiagStatus;
```

**의미:**
- `isSecurityUnlock = TRUE` → 인증 완료
- WriteData(0x2E)는 이 플래그를 확인하고 실행 여부 결정

---

## 8️⃣ 세션 제약 확인 (Table 23)

### RequestSeed & SendKey는 Extended만!

```c
if(stBswDcmData.stDiagStatus.currentSession == DIAG_SESSION_DEFAULT)
{
    errorResult = NRC_ServiceNotSupportedInActiveSession;
    // ← "Default 세션에서는 안 돼!"
}
```

**강의록 P.6 Table 23 (진단 적응 표):**

| Service | Default | Extended | Notes |
|---------|---------|----------|-------|
| DiagSessionControl(0x10) | O | O | 어디서나 가능 |
| ECUReset(0x11) | O | O | 어디서나 가능 |
| SecurityAccess(0x27) | **✗** | **✓** | Extended만! |
| WriteData(0x2E) | ✗ | ✓ (인증 후) | Extended + 인증 |

**왜?**
```
Default 세션:
  → 기본 조회/진단만 가능
  → "공개된 정보만 봐"
  
Extended 세션 + SecurityAccess:
  → 중요한 데이터 수정 가능
  → "인증된 진단기만 들어와"
```

**참고:** 강의록 **P.6 "Table 23"**, 이미지 `10_image_0.png`

---

## 9️⃣ NRC (Negative Response Code) 정리

### SecurityAccess에서 나올 수 있는 에러

| NRC | 16진 | 의미 | 상황 |
|-----|------|------|------|
| ServiceNotSupportedInActiveSession | 0x7E | 세션 부적합 | Default 세션에서 요청 |
| IncorrectMessageLengthOrInvalidFormat | 0x13 | 길이 오류 | RequestSeed: != 2바이트, SendKey: != 4바이트 |
| InvalidKey | 0x35 | 틀린 Key | SendKey에서 0x5678 != 계산값 |
| SubFunctionNotSupported | 0x12 | 미지원 SubFunc | SubFunc != 0x01 and != 0x02 |

**각 에러별 응답:**
```
에러 발생:
[0x03] [0x7F] [SID] [NRC]
 길이    Neg   원본   에러
        Resp   SID   코드

예: RequestSeed를 Default 세션에서
RX [7DB] 03 7F 27 7E AA AA AA AA
         길이 NegResp SID NRC(0x7E)
```

**참고:** 강의록 **P.28 "Table 40"**: 요청 메시지 정의, NRC 리스트

---

## 🔟 요약: UDS_2 vs UDS_3

| 항목 | UDS_1 | UDS_2 | UDS_3 |
|------|-------|-------|-------|
| **추가 서비스** | DiagSessionControl | ECUReset | SecurityAccess |
| **목적** | 세션 전환 | 하드웨어 리셋 | 인증 (Seed/Key) |
| **SubFunc** | 0x01, 0x03 | 0x01 | 0x01(Seed), 0x02(Key) |
| **응답 길이** | 6바이트 | 2바이트 | 4바이트(Seed) or 2바이트(Key) |
| **상태 변수** | currentSession | +isEcuReset | +isSecurityUnlock |
| **세션 제약** | 양쪽 O | 양쪽 O | Extended만 |
| **OTA 단계** | 1단계 | 마지막(5) | 중간(4) |

---

## 🎓 이해 체크리스트

- [ ] SecurityAccess의 2단계 구조 이해 (RequestSeed → SendKey)
- [ ] Seed/Key가 왜 필요한지 알기 (진단기 인증)
- [ ] 16비트 Seed 조합: `(High << 8) | Low` 계산 가능
- [ ] CAN 메시지 포맷 읽기: `[길이|0x27|SubFunc][Data...]`
- [ ] Table 23 세션 제약: **Extended만!**
- [ ] NRC 에러코드 구분: 0x7E, 0x35, 0x13, 0x12

---

**강의록 전체 참고:**
- **P.28**: SecurityAccess 메시지 정의 (Table 40/41)
- **P.30**: 실제 CAN 로그 (Seed/Key 테이블)
- **P.5-6**: Table 23 세션 제약
- **이미지**: 28_image_0.png, 30_image_0.png, 30_image_1.png
