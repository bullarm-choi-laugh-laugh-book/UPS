# UDS_7 분석: WriteDataByIdentifier (0x2E)

## 📌 개요
- **변경 파일**: `Rte_BswDcm.c` (UDS_6 → UDS_7)
- **핵심 기능**: ECU 데이터 쓰기 (NVM에 VariantCode 저장)
- **관련 UDS 서비스 ID**: 0x2E
- **강의록 위치**: P.XX "WriteDataByIdentifier" (확인 예정)

---

## 1️⃣ 코드 비교: UDS_6 → UDS_7

### 📝 [1] SID 및 DID (Data Identifier) 정의 추가

#### **UDS_6 (기존 - WriteDataByIdentifier 없음)**
```c
#define SID_DiagSessionControl				0x10u
#define SID_EcuReset						0x11u
#define SID_SecurityAccess					0x27u
#define SID_CommunicationControl			0x28u
#define SID_RoutineControl					0x31u
```

#### **UDS_7 (새 코드 - WriteDataByIdentifier 추가)**
```c
#define SID_DiagSessionControl				0x10u
#define SID_EcuReset						0x11u
#define SID_SecurityAccess					0x27u
#define SID_CommunicationControl			0x28u
#define SID_RoutineControl					0x31u
#define SID_WriteDataByIdentifier			0x2Eu  // ✅ NEW

#define DID_VariantCode						0xF101u  // ✅ NEW (2바이트)
```

---

### 📝 [2] NVM 관련 상수 추가

#### **UDS_6 (기존)**
```c
// NVM 관련 정의 없음
```

#### **UDS_7 (새 코드 - NVM 데이터 정의 추가)**
```c
#define DATA_LEN_VariantCode 4u     // ✅ NEW: VariantCode는 4바이트
#define DATA_ID_VariantCode  0x01u  // ✅ NEW: NVM ID
```

---

### 📝 [3] 함수 선언 추가

#### **UDS_6 (기존)**
```c
static uint8 diagnosticSessionControl(void);
static uint8 ecuReset(void);
static uint8 securityAccess(void);
static uint8 communicationControl(void);
static uint8 routineControl(void);
```

#### **UDS_7 (새 코드 - writeDataByIdentifier() 함수 선언 추가)**
```c
static uint8 diagnosticSessionControl(void);
static uint8 ecuReset(void);
static uint8 securityAccess(void);
static uint8 communicationControl(void);
static uint8 routineControl(void);
static uint8 writeDataByIdentifier(void);  // ✅ NEW 함수 선언
```

---

### 📝 [4] REtBswDcm_bswDcmMain() 함수 - 서비스 라우팅 확장

#### **UDS_6 (기존 - 5개 서비스 처리)**
```c
void REtBswDcm_bswDcmMain(void)
{
	// ...
	if(serviceId == SID_DiagSessionControl) { /* ... */ }
	else if(serviceId == SID_EcuReset) { /* ... */ }
	else if(serviceId == SID_SecurityAccess) { /* ... */ }
	else if(serviceId == SID_CommunicationControl) { /* ... */ }
	else if(serviceId == SID_RoutineControl) { /* ... */ }
	else { errorResult = NRC_ServiceNotSupported; }
	// ...
}
```

#### **UDS_7 (새 코드 - WriteDataByIdentifier 라우팅 추가)**
```c
void REtBswDcm_bswDcmMain(void)
{
	// ...
	if(serviceId == SID_DiagSessionControl) { /* ... */ }
	else if(serviceId == SID_EcuReset) { /* ... */ }
	else if(serviceId == SID_SecurityAccess) { /* ... */ }
	else if(serviceId == SID_CommunicationControl) { /* ... */ }
	else if(serviceId == SID_RoutineControl) { /* ... */ }
	else if(serviceId == SID_WriteDataByIdentifier)  // ✅ NEW 라우팅
	{
		errorResult = writeDataByIdentifier();
	}
	else { errorResult = NRC_ServiceNotSupported; }
	// ...
}
```

---

### 📝 [5] writeDataByIdentifier() 함수 - 완전히 새로운 구현

#### **UDS_6 (없음)**
```c
// writeDataByIdentifier() 함수 없음
```

#### **UDS_7 (새 코드 - 완전히 새로운 함수 추가)**
```c
static uint8 writeDataByIdentifier(void)
{
	uint8 variantCode[DATA_LEN_VariantCode] = {0u,};
	uint8 errorResult = FALSE;
	
	//========================================
	// ✅ 계산: Data Identifier 추출 (Big-Endian)
	//========================================
	uint16 dataIdentifier = ((uint16_t)stBswDcmData.diagDataBufferRx[1] << 8) 
	                       | (uint16_t)stBswDcmData.diagDataBufferRx[2];
	                       // [1]=Upper, [2]=Lower

	//========================================
	// ✅ 검증 단계 1: 세션 (ExtendedSession만 허용)
	//========================================
	if(stBswDcmData.stDiagStatus.currentSession != DIAG_SESSION_EXTENDED)
	{
		errorResult = NRC_ServiceNotSupportedInActiveSession;
	}
	//========================================
	// ✅ 검증 단계 2: 보안 (SecurityUnlock 필수)
	//========================================
	else if(stBswDcmData.stDiagStatus.isSecurityUnlock == FALSE)
	{
		errorResult = NRC_SecurityAccessDenied;
	}
	//========================================
	// ✅ 실행 단계: DID별 처리 - VariantCode
	//========================================
	else if(dataIdentifier == DID_VariantCode)  // 0xF101
	{
		// ✅ 검증 단계 3: 데이터 길이 (정확히 7바이트)
		// [SID(1)] + [DID_Hi(1)] + [DID_Lo(1)] + [Data0~3(4)] = 7
		if(stBswDcmData.diagDataLengthRx != 7u)
		{
			errorResult = NRC_IncorrectMessageLengthOrInvalidFormat;
		}
		else
		{
			// ✅ 데이터 추출 (4바이트)
			variantCode[0] = stBswDcmData.diagDataBufferRx[3];
			variantCode[1] = stBswDcmData.diagDataBufferRx[4];
			variantCode[2] = stBswDcmData.diagDataBufferRx[5];
			variantCode[3] = stBswDcmData.diagDataBufferRx[6];
			
			// ✅ NVM에 데이터 쓰기 (중요!)
			Rte_Call_BswDcm_rEcuAbsNvm_writeNvmData(DATA_ID_VariantCode, variantCode);
			
			// ✅ 성공 응답 구성 (길이: 3바이트)
			stBswDcmData.diagDataLengthTx = 3u;
			stBswDcmData.diagDataBufferTx[0] = (FRAME_TYPE_SINGLE << 4u) | stBswDcmData.diagDataLengthTx;
			stBswDcmData.diagDataBufferTx[1] = SID_WriteDataByIdentifier + 0x40u;  // 0x6E
			stBswDcmData.diagDataBufferTx[2] = (uint8)((dataIdentifier & 0xFF00) >> 8u);   // DID Upper
			stBswDcmData.diagDataBufferTx[3] = (uint8)(dataIdentifier & 0xFFu);            // DID Lower
		}
	}
	//========================================
	// ✅ 에러: 지원 안 하는 DID
	//========================================
	else
	{
		errorResult = NRC_RequestOutRange;
	}
	
	return errorResult;
}
```

---

## 2️⃣ 함수 상세 분석

### writeDataByIdentifier() 핵심 로직

#### **요청/응답 메시지 포맷**
```
[요청]
Byte 0: [FrameType:0 | Length:7]
Byte 1: SID 0x2E
Byte 2: DID Upper (Big-Endian)
Byte 3: DID Lower
Byte 4-7: Data[0-3] (VariantCode = 4바이트)

[응답]
Byte 0: [FrameType:0 | Length:3]
Byte 1: SID+0x40 = 0x6E
Byte 2: DID Upper Echo
Byte 3: DID Lower Echo
```

#### **Data Identifier (DID) 종류**
| DID | 값 | 의미 | 데이터 크기 | 용도 |
|-----|-----|------|-----------|------|
| DID_VariantCode | 0xF101 | 차량 변형 코드 | 4바이트 | OTA 후 펌웨어 버전 저장 |

#### **검증 체계 (3단계)**
1. **세션 검증**: ExtendedSession만 가능
2. **보안 검증**: SecurityUnlock 필수
3. **DID별 추가 검증**: VariantCode는 길이 7바이트 고정

---

## 3️⃣ 프로토콜 동작 및 NVM 저장

### 검증 흐름

```
요청 도착 (길이:7)
    ↓
[검증 1] 세션 = Extended?
    NO → NRC_ServiceNotSupportedInActiveSession
    ↓ YES
[검증 2] SecurityUnlock = TRUE?
    NO → NRC_SecurityAccessDenied
    ↓ YES
[검증 3] DID = 0xF101?
    NO → NRC_RequestOutRange
    ↓ YES
[검증 4] 길이 = 7?
    NO → NRC_IncorrectMessageLengthOrInvalidFormat
    ↓ YES
[실행] NVM에 4바이트 데이터 저장
    ↓
    Rte_Call_BswDcm_rEcuAbsNvm_writeNvmData()
    ↓
응답 송신 (길이:3)
```

### OTA 흐름에서의 역할

```
[1단계] DiagSessionControl → ExtendedSession
[2단계] SecurityAccess → 인증
[3단계] CommunicationControl(Disable) → 통신 OFF
[4단계] (Flash 작업)
[5단계] CommunicationControl(Enable) → 통신 ON
[6단계] RoutineControl → AliveCounter 모드 설정
[7단계] (몇 초 대기)
[8단계] WriteDataByIdentifier (0x2E) ← ✅ 펌웨어 버전 정보 저장
        ↓
        VariantCode (4바이트) → NVM에 저장
        ↓
        ECU 재시작 후에도 보존
[9단계] EcuReset (0x11) → 재부팅
```

**VariantCode의 의미:**
- 새로 설치된 펌웨어의 버전/변형 코드
- NVM에 영구 저장 → 재부팅 후에도 유지
- 향후 ReadDataByIdentifier로 읽어서 검증 가능

---

## 4️⃣ 고급 기능: NVM 저장

### NVM (Non-Volatile Memory) 호출

```c
Rte_Call_BswDcm_rEcuAbsNvm_writeNvmData(DATA_ID_VariantCode, variantCode);
```

**역할:**
- ECU의 비휘발성 메모리에 데이터 저장
- Flash 메모리의 특정 영역 (NVM 섹터)에 기록
- 전원을 꺼도 데이터 유지

**용도:**
- OTA 후 펌웨어 버전 저장
- 향후 진단기에서 ReadDataByIdentifier로 확인

---

## 5️⃣ UDS_6 vs UDS_7 비교

| 항목 | UDS_6 (RoutineControl 0x31) | UDS_7 (WriteDataByIdentifier 0x2E) |
|------|---|---|
| **서비스 목적** | 루틴 실행 | **데이터 쓰기** |
| **서비스 ID** | 0x31 | **0x2E** |
| **요청 길이** | 4바이트 | **7바이트** (VariantCode) |
| **응답 길이** | 4바이트 | **3바이트** |
| **파라미터** | RID (2바이트) | **DID (2바이트) + Data (4바이트)** |
| **세션 검증** | 없음 | **있음 (ExtendedSession)** |
| **보안 검증** | 없음 | **있음 (SecurityUnlock)** |
| **핵심 작업** | 상태 플래그 설정 | **NVM (Flash) 저장** |
| **응답 구조** | RID Echo | **DID Echo 만** |

---

## 📌 핵심 포인트 

1. **7바이트 요청**: SID + DID(2) + Data(4) = 7바이트
2. **3바이트 응답**: SID + DID(2) Echo 만
3. **2단계 선행 검증**: 세션(Extended) → 보안(Unlock)
4. **DID 기반**: 어떤 데이터를 쓸지 DID로 식별
5. **NVM 저장**: 진단 서비스에서 직접 Flash에 기록
6. **영구성**: 전원 재인가 후에도 데이터 유지
7. **Big-Endian**: DID도 2바이트 Big-Endian 형식
