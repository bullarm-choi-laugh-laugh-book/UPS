# UDS_8 분석: ReadDataByIdentifier (0x22)

## 📌 개요
- **변경 파일**: `Rte_BswDcm.c` (UDS_7 → UDS_8)
- **핵심 기능**: ECU 데이터 읽기 (NVM의 VariantCode 조회)
- **관련 UDS 서비스 ID**: 0x22
- **강의록 위치**: P.XX "ReadDataByIdentifier" (확인 예정)

---

## 1️⃣ 코드 비교: UDS_7 → UDS_8

### 📝 [1] SID 정의 추가

#### **UDS_7 (기존 - ReadDataByIdentifier 없음)**
```c
#define SID_DiagSessionControl				0x10u
#define SID_EcuReset						0x11u
#define SID_SecurityAccess					0x27u
#define SID_CommunicationControl			0x28u
#define SID_RoutineControl					0x31u
#define SID_WriteDataByIdentifier			0x2Eu
```

#### **UDS_8 (새 코드 - ReadDataByIdentifier 추가)**
```c
#define SID_DiagSessionControl				0x10u
#define SID_EcuReset						0x11u
#define SID_SecurityAccess					0x27u
#define SID_CommunicationControl			0x28u
#define SID_RoutineControl					0x31u
#define SID_WriteDataByIdentifier			0x2Eu
#define SID_ReadDataByIdentifier			0x22u  // ✅ NEW
```

---

### 📝 [2] 함수 선언 추가

#### **UDS_7 (기존)**
```c
static uint8 diagnosticSessionControl(void);
static uint8 ecuReset(void);
static uint8 securityAccess(void);
static uint8 communicationControl(void);
static uint8 routineControl(void);
static uint8 writeDataByIdentifier(void);
```

#### **UDS_8 (새 코드 - readDataByIdentifier() 함수 선언 추가)**
```c
static uint8 diagnosticSessionControl(void);
static uint8 ecuReset(void);
static uint8 securityAccess(void);
static uint8 communicationControl(void);
static uint8 routineControl(void);
static uint8 writeDataByIdentifier(void);
static uint8 readDataByIdentifier(void);  // ✅ NEW 함수 선언
```

---

### 📝 [3] REtBswDcm_bswDcmMain() 함수 - 서비스 라우팅 확장

#### **UDS_7 (기존 - 6개 서비스 처리)**
```c
void REtBswDcm_bswDcmMain(void)
{
	// ...
	if(serviceId == SID_DiagSessionControl) { /* ... */ }
	else if(serviceId == SID_EcuReset) { /* ... */ }
	else if(serviceId == SID_SecurityAccess) { /* ... */ }
	else if(serviceId == SID_CommunicationControl) { /* ... */ }
	else if(serviceId == SID_RoutineControl) { /* ... */ }
	else if(serviceId == SID_WriteDataByIdentifier) { /* ... */ }
	else { errorResult = NRC_ServiceNotSupported; }
	// ...
}
```

#### **UDS_8 (새 코드 - ReadDataByIdentifier 라우팅 추가)**
```c
void REtBswDcm_bswDcmMain(void)
{
	// ...
	if(serviceId == SID_DiagSessionControl) { /* ... */ }
	else if(serviceId == SID_EcuReset) { /* ... */ }
	else if(serviceId == SID_SecurityAccess) { /* ... */ }
	else if(serviceId == SID_CommunicationControl) { /* ... */ }
	else if(serviceId == SID_RoutineControl) { /* ... */ }
	else if(serviceId == SID_WriteDataByIdentifier) { /* ... */ }
	else if(serviceId == SID_ReadDataByIdentifier)  // ✅ NEW 라우팅
	{
		errorResult = readDataByIdentifier();
	}
	else { errorResult = NRC_ServiceNotSupported; }
	// ...
}
```

---

### 📝 [4] readDataByIdentifier() 함수 - 완전히 새로운 구현

#### **UDS_7 (없음)**
```c
// readDataByIdentifier() 함수 없음
```

#### **UDS_8 (새 코드 - 완전히 새로운 함수 추가)**
```c
static uint8 readDataByIdentifier(void)
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
	// ✅ 검증 단계 1: 메시지 길이 (3바이트 고정 - SID+DID만)
	//========================================
	if(stBswDcmData.diagDataLengthRx != 3u)
	{
		errorResult = NRC_IncorrectMessageLengthOrInvalidFormat;
	}
	//========================================
	// ✅ 실행 단계: DID별 처리 - VariantCode
	//========================================
	else if(dataIdentifier == DID_VariantCode)  // 0xF101
	{
		// ✅ NVM에서 데이터 읽기 (중요!)
		Rte_Call_BswDcm_rEcuAbsNvm_readNvmData(DATA_ID_VariantCode, variantCode);
		
		// ✅ 성공 응답 구성 (길이: 7바이트)
		stBswDcmData.diagDataLengthTx = 7u;
		stBswDcmData.diagDataBufferTx[0] = (FRAME_TYPE_SINGLE << 4u) | stBswDcmData.diagDataLengthTx;
		stBswDcmData.diagDataBufferTx[1] = SID_ReadDataByIdentifier + 0x40u;  // 0x62
		stBswDcmData.diagDataBufferTx[2] = (uint8)((dataIdentifier & 0xFF00) >> 8u);  // DID Upper
		stBswDcmData.diagDataBufferTx[3] = (uint8)(dataIdentifier & 0xFFu);           // DID Lower
		// DATA 복사 (4바이트)
		stBswDcmData.diagDataBufferTx[4] = variantCode[0];
		stBswDcmData.diagDataBufferTx[5] = variantCode[1];
		stBswDcmData.diagDataBufferTx[6] = variantCode[2];
		stBswDcmData.diagDataBufferTx[7] = variantCode[3];
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

### readDataByIdentifier() 핵심 로직

#### **요청/응답 메시지 포맷**
```
[요청] (3바이트만)
Byte 0: [FrameType:0 | Length:3]
Byte 1: SID 0x22
Byte 2: DID Upper (Big-Endian)
Byte 3: DID Lower

[응답] (7바이트)
Byte 0: [FrameType:0 | Length:7]
Byte 1: SID+0x40 = 0x62
Byte 2: DID Upper Echo
Byte 3: DID Lower Echo
Byte 4-7: Data[0-3] (VariantCode = 4바이트)
```

#### **비교: Write vs Read**
| 항목 | Write (0x2E) | Read (0x22) |
|------|---|---|
| **요청 길이** | 7바이트 | **3바이트** |
| **응답 길이** | 3바이트 | **7바이트** |
| **요청 데이터** | SID + DID + Data(4) | **SID + DID만** |
| **응답 데이터** | SID + DID Echo | **SID + DID + Data(4)** |
| **역할** | 데이터 쓰기 | **데이터 읽기** |
| **검증** | 3단계 (세션, 보안, 길이) | **1단계 (길이만)** |

---

## 3️⃣ 프로토콜 동작 및 NVM 읽기

### 검증 흐름 (단순함!)

```
요청 도착 (길이:3)
    ↓
[검증 1] 길이 = 3?
    NO → NRC_IncorrectMessageLengthOrInvalidFormat
    ↓ YES
[검증 2] DID = 0xF101?
    NO → NRC_RequestOutRange
    ↓ YES
[실행] NVM에서 데이터 읽기
    ↓
    Rte_Call_BswDcm_rEcuAbsNvm_readNvmData()
    ↓
응답 송신 (길이:7, DID + Data 포함)
```

**특징:**
- Write와 달리 세션/보안 검증 **없음** (읽기는 누구나 가능)
- 길이 검증만 함 (3바이트 = SID + DID 2바이트)
- NVM에서 데이터를 읽어서 응답에 담음

### OTA 흐름에서의 역할

```
[1단계] DiagSessionControl → ExtendedSession
[2단계] SecurityAccess → 인증
[3단계] CommunicationControl(Disable) → 통신 OFF
[4단계] (Flash 작업)
[5단계] CommunicationControl(Enable) → 통신 ON
[6단계] RoutineControl → AliveCounter 모드 설정
[7단계] (2~3초 대기)
[8단계] WriteDataByIdentifier (0x2E) → VariantCode 저장
[9단계] ReadDataByIdentifier (0x22) ← ✅ 저장된 버전 확인!
        ↓
        "정말 우리가 저장한 VariantCode가 있는가?"
        ↓
        응답에 포함된 Data가 예상값과 일치? ✅
[10단계] EcuReset (0x11) → 재부팅
```

**역할:**
- OTA 최종 검증: 저장한 데이터가 정말 NVM에 있는가?
- 진단기가 성공 여부를 확인
- 향후 런타임에도 "어떤 버전이 설치되었는가" 확인 가능

---

## 4️⃣ NVM 읽기 (Write의 역함수)

### NVM 호출 비교

**Write (저장):**
```c
Rte_Call_BswDcm_rEcuAbsNvm_writeNvmData(DATA_ID_VariantCode, variantCode);
```

**Read (읽기):**
```c
Rte_Call_BswDcm_rEcuAbsNvm_readNvmData(DATA_ID_VariantCode, variantCode);
```

**결과:**
- `variantCode[]` 배열에 NVM의 값이 채워짐
- 응답 메시지에 넣어서 진단기로 전송

---

## 5️⃣ UDS_7 vs UDS_8 비교

| 항목 | UDS_7 (WriteDataByIdentifier 0x2E) | UDS_8 (ReadDataByIdentifier 0x22) |
|------|---|---|
| **서비스 목적** | 데이터 쓰기 → NVM 저장 | **데이터 읽기 ← NVM 조회** |
| **서비스 ID** | 0x2E | **0x22** |
| **요청 길이** | 7바이트 (SID+DID+Data) | **3바이트 (SID+DID만)** |
| **응답 길이** | 3바이트 | **7바이트 (SID+DID+Data)** |
| **검증 단계** | 3단계 (세션, 보안, 길이) | **1단계 (길이만)** |
| **세션 검증** | 있음 (ExtendedSession) | **없음** |
| **보안 검증** | 있음 (SecurityUnlock) | **없음** |
| **NVM 작업** | 쓰기 (writeNvmData) | **읽기 (readNvmData)** |
| **응답 구조** | DID Echo만 | **DID Echo + Data(4)** |

---

## 📌 핵심 포인트

1. **3바이트 요청**: SID + DID (2바이트) = 매우 간단
2. **7바이트 응답**: SID + DID(2) + Data(4) = Write의 역구조
3. **검증 최소화**: 길이만 확인, 세션/보안 없음
4. **NVM 읽기**: readNvmData()로 Flash에서 데이터 추출
5. **OTA 검증**: WriteDataByIdentifier 후 Read로 정말 저장되었는지 확인
6. **누구나 가능**: 읽기는 권한 제한 없음 (Write는 UnlockRequired)
7. **Big-Endian**: DID도 2바이트 Big-Endian 형식
8. **최종 확인**: ReadDataByIdentifier 성공 = OTA 완전 성공! ✅
