# UDS_2 분석: ECUReset (0x11)

## 📌 개요
- **변경 파일**: `Rte_BswDcm.c` (UDS_1에서 추가 확장)
- **핵심 기능**: 하드웨어 리셋 제어 (ECU 강제 재부팅)
- **관련 UDS 서비스 ID**: 0x11
- **강의록 위치**: P.23-25 "Uds : Ecureset"
- **관련 이미지**: `23_image_0.png`, `24_image_0.png`, `25_image_0.png`, `25_image_1.png`, `25_image_2.png`, `25_image_3.png`

---

## 1️⃣ 코드 비교: UDS_1 → UDS_2

### 📝 [1] 상수 정의 및 함수 선언 추가

#### **UDS_1 (이전)**
```c
#define SID_DiagSessionControl				0x10u
// ↑ 세션 제어만 정의
```

#### **UDS_2 새로 추가됨**
```c
#define SID_DiagSessionControl				0x10u
#define SID_EcuReset						0x11u		// ✅ NEW: ECUReset 서비스 ID

#define SFID_DiagnosticSessionControl_DefaultSession	0x01u
#define SFID_DiagnosticSessionControl_ExtendedSession	0x03u
#define SFID_EcuReset_HardReset 					0x01u	// ✅ NEW: HardReset 서브함수

// ...

static uint8 diagnosticSessionControl(void);
static uint8 ecuReset(void);					// ✅ NEW: ecuReset 함수 선언
```

**참고 자료:**
- 강의록 P.24 "Table 33 - Request message definition"에서 SID 0x11 정의
- 강의록 P.24 "Table 34 - Request message sub-function parameter definition"에서 0x01 = hardReset 정의
- 이미지 `23_image_0.png`: ECUReset 개요

---

### 📝 [2] REtBswDcm_bswDcmMain() 확장

#### **UDS_1 (이전)**
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
		else  // ← UDS_0x10만 처리 가능
		{
			errorResult = NRC_ServiceNotSupported;
		}
		// ... 응답 처리 ...
	}
}
```

#### **UDS_2 새로 추가됨**
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
		else if(serviceId == SID_EcuReset)  // ✅ NEW: 0x11 추가
		{
			errorResult = ecuReset();       // ✅ NEW: ecuReset() 호출
		}
		else
		{
			errorResult = NRC_ServiceNotSupported;
		}
		
		// ... 응답 처리 (동일) ...
	}
}
```

**의미:**
- UDS_1에서는 0x10(세션제어)만 처리했음
- UDS_2에서는 0x11(ECUReset)을 추가로 처리 가능
- 나머지 로직은 동일 (에러 응답, 패딩)

**참고 자료:**
- 강의록 P.24 코드 예시: serviceId == SID_EcuReset 검사 추가

---

### 📝 [3] REoiBswDcm_pDcmCmd_getDiagStatus() 구현

#### **UDS_1 (이전 - 미구현)**
```c
void REoiBswDcm_pDcmCmd_getDiagStatus(P_stDiagStatus pstDiagStatus)
{
    (void) 0;  // ← 구현 안 됨: 진단 상태 반환 불가
}
```

#### **UDS_2 새로 구현됨**
```c
void REoiBswDcm_pDcmCmd_getDiagStatus(P_stDiagStatus pstDiagStatus)
{
	// ✅ NEW: 현재 진단 상태를 호출자에게 복사해서 반환
	(void)memcpy(pstDiagStatus, &stBswDcmData.stDiagStatus, sizeof(ST_DiagStatus));
	//                                                                  ↑
	//                                                    구조체 전체 크기만큼 복사
}
```

**의미 및 역할:**
```
다른 모듈에서 호출:
    └─ "현재 세션이 뭐야? ECU 리셋 상태는?"
    
→ REoiBswDcm_pDcmCmd_getDiagStatus() 호출
    └─ stBswDcmData.stDiagStatus의 내용을 복사
    
→ 호출자가 분석:
    ├─ currentSession: 0 = DEFAULT, 1 = EXTENDED
    └─ isEcuReset: 1 = 리셋 명령 받음
```

**실제 사용처:**
- 강의록 P.25, 이미지 `25_image_3.png`
```c
void REtEcuAbs_ecuAbsMain(void)
{
    ST_DiagStatus stDiagStatus;
    // ... 초기화 ...
    
    // ✅ 현재 진단 상태 조회
    Rte_Call_EcuAbs_rDcmCmd_getDiagStatus(&stDiagStatus);
    
    // ✅ ECU 리셋 확인
    if(stDiagStatus.isEcuReset == TRUE) {
        Mcu_PerformReset();  // ← 실제 리셋 실행
    }
}
```

**참고 자료:**
- 강의록 P.24 코드: `(void)memcpy(pstDiagStatus, ...);` 구현
- 이미지 `25_image_0.png`, `25_image_1.png`, `25_image_2.png`: 리셋 흐름 다이어그램
- 이미지 `25_image_3.png`: EcuAbs에서 호출하는 예시

---

### 📝 [4] ecuReset() 함수 (새로 추가)

#### **함수 정의 및 구현**

```c
// ✅ NEW: ECUReset 서비스 0x11 구현
static uint8 ecuReset(void)
{
	// ✅ Step 1: 서브함수 ID 추출
	uint8 subFunctionId = stBswDcmData.diagDataBufferRx[1] & 0x7Fu;
	uint8 errorResult = FALSE;

	// ✅ Step 2: 요청 메시지 길이 검증 (정확히 2바이트)
	// [Byte#1: SID=0x11] [Byte#2: SubFunc=0x01]
	if (stBswDcmData.diagDataLengthRx != 2u)
	{
		errorResult = NRC_IncorrectMessageLengthOrInvalidFormat;
	}
	// ✅ Step 3: HardReset 서브함수 처리
	else if(subFunctionId == SFID_EcuReset_HardReset)  // 0x01
	{
		// ✅ KEY: 리셋 플래그 설정 (실제 리셋은 EcuAbs에서 실행)
		stBswDcmData.stDiagStatus.isEcuReset = TRUE;
		//                                                ↑
		//                           리셋 요청을 상태에 기록
	}
	// ✅ Step 4: 지원 안 하는 서브함수 (0x02, 0x03 등)
	else
	{
		errorResult = NRC_SubFunctionNotSupported;
	}
	
	// ✅ Step 5: 성공 시 긍정 응답 구성
	if(errorResult == FALSE)
	{
		// 응답 포맷: [프레임|길이] [0x51] [0x01]
		// 길이 = 2 (응답 SID + 서브함수)
		stBswDcmData.diagDataLengthTx = 2u;
		
		// Byte #0: [SINGLE(0)|길이2]
		stBswDcmData.diagDataBufferTx[0] = (FRAME_TYPE_SINGLE << 4u) | stBswDcmData.diagDataLengthTx;
		
		// Byte #1: 0x51 = 0x11 + 0x40 (긍정 응답)
		stBswDcmData.diagDataBufferTx[1] = SID_EcuReset + 0x40u;  // 0x11 + 0x40 = 0x51
		
		// Byte #2: 서브함수 에코 (요청한 것과 동일)
		stBswDcmData.diagDataBufferTx[2] = subFunctionId;
	}
	
	return errorResult;  // 0 = 성공, 0x?? = 에러
}
```

**실행 흐름 다이어그램:**

```
진단기에서 ECU 리셋 요청
    ↓
[CAN RX] 0x02 0x11 0x01 [Filler:0x55...]
           ↑    ↑   ↑
         길이 SID SubFunc(HardReset)
    ↓
REoiBswDcm_pDcmCmd_processDiagRequest()
└─ stBswDcmData에 저장 + 플래그 설정
    ↓
REtBswDcm_bswDcmMain()
└─ 0x11 확인 → ecuReset() 호출
    ↓
ecuReset()
├─ 길이 검증: 2바이트 OK
├─ 서브함수: 0x01 (HardReset) OK
└─ isEcuReset = TRUE ← 핵심!
    ↓
응답 메시지 생성 & 전송
[CAN TX] 0x02 0x51 0x01 [Filler:0xAA...]
           ↑    ↑   ↑
         길이 응답  에코
    ↓
진단기가 응답 수신
    ↓
EcuAbs (메인 루프)
└─ getDiagStatus() → isEcuReset 확인
   └─ TRUE → Mcu_PerformReset() 실행
      ↓
ECU 하드웨어 리셋!!! (전원 재부팅과 동일)
```

**참고 자료:**
- 강의록 P.24 "Table 33": Request message SID=0x11, SubFunc
- 강의록 P.24 "Table 34": SubFunc 0x01 = hardReset 정의
- 강의록 P.24-25 코드 예시: ecuReset() 함수 구현
- 강의록 P.25 "Table 35": Response SID=0x51, SubFunc

---

## 2️⃣ UDS_1 vs UDS_2: 변화 요약

| 항목 | UDS_1 | UDS_2 | 변화 |
|------|-------|-------|------|
| **서비스** | 0x10만 | 0x10, **0x11** | ✅ ECUReset 추가 |
| **상수** | SID_DiagSessionControl | + SID_EcuReset, SFID_EcuReset_HardReset | ✅ 2개 추가 |
| **함수** | diagnosticSessionControl | + ecuReset | ✅ 함수 추가 |
| **getDiagStatus** | (void)0 (미구현) | memcpy로 상태 반환 | ✅ 구현됨 |
| **REtBswDcm_bswDcmMain** | if-else (0x10만) | if-else-if (0x10, **0x11**) | ✅ 분기 추가 |
| **기능** | 세션 전환 | 세션 전환 + **하드 리셋** | ✅ 리셋 기능 |

---

## 3️⃣ 핵심 개념: 왜 ECUReset이 필요한가?

### 🎯 리셋의 역할

```
OTA 플로우:
1. [UDS_1] 세션 전환: Default → Extended
2. [UDS_3] 보안 인증: Seed/Key 교환
3. [UDS_7] 데이터 쓰기: NVM 또는 Flash 업데이트
4. [UDS_2] ← ★ 리셋!!! (새 펌웨어 로드를 위해)
5. [부팅]  새 펌웨어 실행
```

**ECUReset이 없으면?**
- 새로운 펌웨어가 메모리에만 저장되고 로드되지 않음
- ECU가 여전히 구 펌웨어 코드 실행
- OTA 실패!

**ECUReset이 하는 일:**
```
┌─────────────────────────────────┐
│  ECU가 실행 중인 상태           │
│  - Program Counter: Old FW      │
│  - RAM: 작업 데이터들           │
└─────────────────────────────────┘
         ↓ Mcu_PerformReset()
┌─────────────────────────────────┐
│  하드웨어 리셋 (전원 재부팅)     │
│  - CPU: 리셋 신호              │
│  - 메모리: 클리어               │
└─────────────────────────────────┘
         ↓ 부팅
┌─────────────────────────────────┐
│  ECU 재부팅 시작                 │
│  - Program Counter: New FW      │
│  - Flash: 새 펌웨어 적용됨      │
└─────────────────────────────────┘
```

---

### 🔄 리셋 메커니즘: 2단계 아키텍처

#### **1. 진단 계층 (BswDcm)**
```c
ecuReset()  // UDS 메시지 처리
{
    // ✅ 단순히 플래그만 설정
    isEcuReset = TRUE;
}
```

#### **2. ECU 관리 계층 (EcuAbs)**
```c
REtEcuAbs_ecuAbsMain()  // 메인 루프
{
    // 초기화, ADC, DIO, SPI, etc
    
    // ✅ 진단 상태 확인
    getDiagStatus(&stDiagStatus);
    
    if(stDiagStatus.isEcuReset == TRUE) {
        // ✅ 실제 리셋 실행
        Mcu_PerformReset();  // MCU 하드웨어 함수
    }
}
```

**왜 이렇게 분리?**
- BswDcm: 진단 프로토콜 처리만 (책임 분리)
- EcuAbs: 실제 HW 제어 (시스템 안정성)
- 장점: 리셋 전에 모든 리소스 안전하게 정리 가능

**참고 자료:**
- 강의록 P.25, 이미지 `25_image_3.png`: EcuAbs 메인 루프에서 호출

---

### 🔐 보안 고려사항

#### **Table 23 (강의록 P.6): 서비스별 세션 제약**

```
┌────────────────────────┬─────────────┬──────────────┐
│ Service                │ Default Sess│ Extended Sess│
├────────────────────────┼─────────────┼──────────────┤
│ 0x10 DiagSessionControl│      ✅     │      ✅      │ ← 항상 가능
│ 0x11 ECUReset          │      ✅     │      ✅      │ ← 항상 가능
│ 0x27 SecurityAccess    │      ❌     │      ✅      │ ← Extended만
│ 0x2E WriteDataByID     │      ❌     │      ✅      │ ← Extended만
└────────────────────────┴─────────────┴──────────────┘
```

**ECUReset이 양쪽 다 가능한 이유:**
- 진단기의 리셋 요청은 정상 기능
- 하지만 실제 데이터 쓰기는 Extended + SecurityAccess 후에만 가능
- 리셋 자체는 아무 데이터도 수정 안 함 (안전함)

**참고 자료:**
- 강의록 P.6 "Table 23 - Services allowed during default and non-default diagnostic session"

---

### 📊 매우 간단한 ECUReset 구현

```c
// 요청:  0x02 0x11 0x01 ...
// 응답:  0x02 0x51 0x01 ...
// 데이터: 최소한!

if(subFunctionId == 0x01) {
    isEcuReset = TRUE;
    → 응답 보내고 끝!
    → 나머지는 메인 루프에서 처리
}
```

**특징:**
- 최소한의 데이터 (길이=2)
- 최소한의 처리 (플래그만 설정)
- 최소한의 응답 (SID + SubFunc + Padding)

---

## 4️⃣ 실제 통신 로그 분석

### 📋 강의록 P.25 실행 로그

```
송신 (Tx):  [7D3] 02 11 01 55 55 55 55
            ├─ 0x02: Single Frame, 길이 2
            ├─ 0x11: ECUReset Service
            ├─ 0x01: HardReset sub-function
            └─ 0x55: Filler bytes (5개)

수신 (Rx):  [7DB] 02 51 01 AA AA AA AA AA
            ├─ 0x02: Single Frame, 길이 2
            ├─ 0x51: 응답 (0x11 + 0x40)
            ├─ 0x01: SubFunc 에코
            └─ 0xAA: Filler bytes
            
↓ 즉시 ECU가 리셋됨!
```

**다음 로그에서 보이는 것:**
```
ECU 재부팅 후 나타나는 신호들:
├─ AliveCount 0x00으로 초기화
├─ SDC_01 메시지 다시 시작
└─ 새 펌웨어 실행 중
```

**참고 자료:**
- 강의록 P.25 테이블: 실제 CAN 로그 (ECU Reset START/PASS)

---

## 5️⃣ UDS_2의 위치 및 역할

### 🗺️ UDS 서비스 순서도

```
OTA Bootloader 흐름:

Step 1: [0x10] DiagSessionControl (UDS_1)
        └─ Default → Extended 전환
        └─ 선행 조건: None (항상 가능)
        
Step 2: [0x11] ECUReset (← UDS_2) ★
        └─ 현재는 선수행 안 함 (나중에 필요)
        └─ 유스케이스: "리셋되었나 확인"
        
Step 3: [0x27] SecurityAccess (UDS_3/4)
        └─ Seed/Key 인증
        └─ 선행 조건: Extended 세션 + 미인증
        
Step 4: [0x2E] WriteDataByIdentifier (UDS_7)
        └─ 데이터 쓰기 (NVM, Flash)
        └─ 선행 조건: Extended + SecurityAccess 성공
        
Step 5: [0x11] ECUReset (← UDS_2 재등장) ★★
        └─ 새 펌웨어 로드를 위해 리셋 강제!
        └─ 이것이 OTA의 마지막 단계!
```

---

## ✅ 결론: UDS_2가 Complete와 다른 이유

1. **기능 추가**: UDS_1(세션)에 UDS_2(리셋) 추가
2. **상태 관리**: `isEcuReset` 플래그로 리셋 요청 추적
3. **계층 분리**: 진단 계층(BswDcm)에서 요청, 시스템 계층(EcuAbs)에서 실행
4. **OTA 필수**: 새 펌웨어 적용 후 **반드시** 리셋해야 함
5. **보안**: 세션 제약 없음 (리셋 자체는 데이터 수정 아님)

**다음 단계 (UDS_3/4):**
- SecurityAccess (0x27): Seed/Key 기반 인증
- 2단계 프로세스: Request Seed (0x01) → Send Key (0x02)
- NVM 접근을 위한 **필수 보안 메커니즘**
