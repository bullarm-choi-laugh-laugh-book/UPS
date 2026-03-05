# UDS_6 분석: RoutineControl (0x31)

## 📌 개요
- **변경 파일**: `Rte_BswDcm.c` (UDS_5 → UDS_6)
- **핵심 기능**: 진단 루틴 실행 제어 (AliveCounter 증감)
- **관련 UDS 서비스 ID**: 0x31
- **강의록 위치**: P.XX "RoutineControl" (확인 예정)

---

## 1️⃣ 코드 비교: UDS_5 → UDS_6

### 📝 [1] SID 및 RID (Routine Identifier) 정의 추가

#### **UDS_5 (기존 - RoutineControl 없음)**
```c
#define SID_DiagSessionControl				0x10u
#define SID_EcuReset						0x11u
#define SID_SecurityAccess					0x27u
#define SID_CommunicationControl			0x28u
```

#### **UDS_6 (새 코드 - RoutineControl 추가)**
```c
#define SID_DiagSessionControl				0x10u
#define SID_EcuReset						0x11u
#define SID_SecurityAccess					0x27u
#define SID_CommunicationControl			0x28u
#define SID_RoutineControl					0x31u  // ✅ NEW

#define SFID_RoutineControl_Start			0x01u   // ✅ NEW

#define RID_AliveCountIncrease				0xF000u  // ✅ NEW (2바이트)
#define RID_AliveCountDecrease				0xF001u  // ✅ NEW (2바이트)
```

---

### 📝 [2] 함수 선언 추가

#### **UDS_5 (기존)**
```c
static uint8 diagnosticSessionControl(void);
static uint8 ecuReset(void);
static uint8 securityAccess(void);
static uint8 communicationControl(void);
```

#### **UDS_6 (새 코드 - routineControl() 함수 선언 추가)**
```c
static uint8 diagnosticSessionControl(void);
static uint8 ecuReset(void);
static uint8 securityAccess(void);
static uint8 communicationControl(void);
static uint8 routineControl(void);  // ✅ NEW 함수 선언
```

---

### 📝 [3] REtBswDcm_bswDcmMain() 함수 - 서비스 라우팅 확장

#### **UDS_5 (기존 - 4개 서비스 처리)**
```c
void REtBswDcm_bswDcmMain(void)
{
	uint8 serviceId;
	uint8 errorResult = FALSE;
	if(stBswDcmData.isServiceRequest == TRUE)
	{
		// ...
		if(serviceId == SID_DiagSessionControl) { /* ... */ }
		else if(serviceId == SID_EcuReset) { /* ... */ }
		else if(serviceId == SID_SecurityAccess) { /* ... */ }
		else if(serviceId == SID_CommunicationControl) { /* ... */ }
		else { errorResult = NRC_ServiceNotSupported; }
		// ...
	}
}
```

#### **UDS_6 (새 코드 - RoutineControl 라우팅 추가)**
```c
void REtBswDcm_bswDcmMain(void)
{
	uint8 serviceId;
	uint8 errorResult = FALSE;
	if(stBswDcmData.isServiceRequest == TRUE)
	{
		// ...
		if(serviceId == SID_DiagSessionControl) { /* ... */ }
		else if(serviceId == SID_EcuReset) { /* ... */ }
		else if(serviceId == SID_SecurityAccess) { /* ... */ }
		else if(serviceId == SID_CommunicationControl) { /* ... */ }
		else if(serviceId == SID_RoutineControl)  // ✅ NEW 라우팅
		{
			errorResult = routineControl();
		}
		else { errorResult = NRC_ServiceNotSupported; }
		// ...
	}
}
```

---

### 📝 [4] routineControl() 함수 - 완전히 새로운 구현

#### **UDS_5 (없음)**
```c
// routineControl() 함수 없음
```

#### **UDS_6 (새 코드 - 완전히 새로운 함수 추가)**
```c
static uint8 routineControl(void)
{
	uint8 errorResult = FALSE;
	uint8 subFunctionId = stBswDcmData.diagDataBufferRx[1];
	
	//========================================
	// ✅ 계산: Routine Identifier 추출 (Big-Endian)
	// 2바이트를 16비트 값으로 변환
	//========================================
	uint16_t routineIdentifier = ((uint16_t)stBswDcmData.diagDataBufferRx[2] << 8) 
	                            | (uint16_t)stBswDcmData.diagDataBufferRx[3];
	                            // [2]=Upper, [3]=Lower

	//========================================
	// ✅ 검증 단계 1: 메시지 길이
	//========================================
	if(stBswDcmData.diagDataLengthRx != 4u)
	{
		errorResult = NRC_IncorrectMessageLengthOrInvalidFormat;
	}
	//========================================
	// ✅ 검증 단계 2: SubFunction (0x01=Start만 허용)
	//========================================
	else if(subFunctionId != SFID_RoutineControl_Start)
	{
		errorResult = NRC_SubFunctionNotSupported;
	}
	//========================================
	// ✅ 실행 단계: RID별 처리 - AliveCountIncrease
	//========================================
	else if (routineIdentifier == RID_AliveCountIncrease)  // 0xF000
	{
		stBswDcmData.stDiagStatus.isAliveCountDecrease = FALSE;  // 증가 모드!
	}
	//========================================
	// ✅ 실행 단계: RID별 처리 - AliveCountDecrease
	//========================================
	else if (routineIdentifier == RID_AliveCountDecrease)  // 0xF001
	{
		stBswDcmData.stDiagStatus.isAliveCountDecrease = TRUE;   // 감소 모드!
	}
	//========================================
	// ✅ 에러: 지원 안 하는 RID
	//========================================
	else
	{
		errorResult = NRC_RequestOutRange;
	}

	//========================================
	// ✅ 성공 응답 구성 (길이: 4바이트)
	//========================================
	if(errorResult == FALSE)
	{
		stBswDcmData.diagDataLengthTx = 4u;
		stBswDcmData.diagDataBufferTx[0] = (FRAME_TYPE_SINGLE << 4u) | stBswDcmData.diagDataLengthTx;
		stBswDcmData.diagDataBufferTx[1] = SID_RoutineControl + 0x40u;  // 0x71
		stBswDcmData.diagDataBufferTx[2] = subFunctionId;               // Echo 0x01
		stBswDcmData.diagDataBufferTx[3] = (uint8)((routineIdentifier & 0xFF00) >> 8u);  // RID Upper
		stBswDcmData.diagDataBufferTx[4] = (uint8)(routineIdentifier & 0xFFu);           // RID Lower
	}
	return errorResult;
}
```

---

## 2️⃣ 함수 상세 분석

### routineControl() 핵심 로직

#### **요청/응답 메시지 포맷**
```
[요청]
Byte 0: [FrameType:0 | Length:4]
Byte 1: SID 0x31
Byte 2: SubFunc 0x01 (Start)
Byte 3: RID Upper (Big-Endian)
Byte 4: RID Lower

[응답]
Byte 0: [FrameType:0 | Length:4]
Byte 1: SID+0x40 = 0x71
Byte 2: SubFunc Echo (0x01)
Byte 3: RID Upper (Echo)
Byte 4: RID Lower (Echo)
```

#### **Big-Endian 변환 (2바이트 파라미터)**
```c
// 요청에서 추출:
routineIdentifier = (Byte[2] << 8) | Byte[3]
                  = (0xF0 << 8) | 0x00
                  = 0xF000

// 응답에서 역변환:
Byte[3] = (routineIdentifier & 0xFF00) >> 8  // Upper
Byte[4] = (routineIdentifier & 0xFFu)        // Lower
```

#### **Routine Identifier (RID) 종류**
| RID | 값 | 의미 | 결과 상태 |
|-----|-----|------|----------|
| RID_AliveCountIncrease | 0xF000 | AliveCounter 증가 모드 | isAliveCountDecrease = FALSE |
| RID_AliveCountDecrease | 0xF001 | AliveCounter 감소 모드 | isAliveCountDecrease = TRUE |

---

## 3️⃣ 프로토콜 동작 및 검증 순서

### 검증 체계 (2+1 단계)

```
요청 도착
    ↓
[검증 1] 길이 = 4바이트?  NO → NRC_IncorrectMessageLengthOrInvalidFormat
    ↓ YES
[검증 2] SubFunc = 0x01?  NO → NRC_SubFunctionNotSupported
    ↓ YES
[실행] RID별 처리
    ↓ RID = 0xF000 → isAliveCountDecrease = FALSE (증가)
    ↓ RID = 0xF001 → isAliveCountDecrease = TRUE (감소)
    ↓ RID = 기타 → NRC_RequestOutRange (미지원)
    ↓
응답 송신 (길이:4, RID Echo 포함)
```

### AliveCounter란?

```
[의미]
ECU가 정상적으로 주기적(예: 10ms)으로 실행되고 있음을 증명하는 카운터
값: 0~255 반복

[OTA 검증에서의 역할]
Flash 쓰기 완료 후...
1. RoutineControl (0x31) → 증가 모드 설정
2. 몇 초 대기
3. ReadDataByIdentifier (0x22) → AliveCounter 값 읽기
4. Counter가 증가했다? → 펌웨어가 실행 중이다! ✅
```

### OTA 흐름에서의 역할

```
[1단계] DiagSessionControl → ExtendedSession
[2단계] SecurityAccess → 인증
[3단계] CommunicationControl(Disable) → 통신 OFF
[4단계] (Flash 작업)
[5단계] CommunicationControl(Enable) → 통신 ON
[6단계] RoutineControl (0x31) ← ✅ 펌웨어 실행 모드 설정
        ↓
        AliveCounter를 증가 모드로 전환
        ↓
[7단계] (2~3초 대기)
[8단계] ReadDataByIdentifier (0x22) → ✅ AliveCounter 값 읽기
        ↓
        증가했다면 = 새 펌웨어가 실행되고 있다! (검증 성공)
[9단계] EcuReset (0x11) → 재부팅
```

---

## 4️⃣ 에러 처리 분석

| 에러 코드 | 상황 | 원인 | 예시 |
|---------|------|------|------|
| NRC_IncorrectMessageLengthOrInvalidFormat | 길이 ≠ 4 | 프로토콜 오류 | [03][31][01][F0] (3바이트) |
| NRC_SubFunctionNotSupported | SubFunc ≠ 0x01 | Start만 지원 | SubFunc=0x02 |
| NRC_RequestOutRange | RID 미지원 | 정의 안 됨 | RID=0x1234 |

---

## 5️⃣ UDS_5 vs UDS_6 비교

| 항목 | UDS_5 (CommunicationControl 0x28) | UDS_6 (RoutineControl 0x31) |
|------|---|---|
| **서비스 목적** | 통신 제어 | **루틴 실행** |
| **서비스 ID** | 0x28 | **0x31** |
| **요청 길이** | 3바이트 | **4바이트** |
| **응답 길이** | 2바이트 | **4바이트** |
| **SubFunction 개수** | 2개 (0x00, 0x03) | **1개 (0x01)** |
| **추가 파라미터** | CommType (1바이트) | **RID (2바이트Big-Endian)** |
| **파라미터 개수** | 2개 (Enable/Disable) | **2개 (Inc/Dec)** |
| **핵심 상태 플래그** | isCommunicationDisable | **isAliveCountDecrease** |
| **응답 구조** | SubFunc Echo만 | **SubFunc+RID Echo** |

---

## 📌 핵심 포인트

1. **4바이트 포맷**: SID + SubFunc + RID(2바이트)
2. **Big-Endian 처리**: 2바이트 값을 상위ByteFirst로 전송
3. **2개 검증**: 길이(4) → SubFunc(0x01)
4. **RID 정의**: 0xF000(증가), 0xF001(감소) 두 가지만 지원
5. **응답 길이**: 4바이트 (RID를 echo하므로 요청과 동일)
6. **AliveCounter**: OTA 완료 후 **실제 실행 검증**의 핵심
7. **세션 무관**: 다른 서비스와 달리 세션 검증 없음
