# UDS_1 분석: DiagnosticSessionControl (0x10)

## 📌 개요
- **변경 파일**: `Rte_BswDcm.c` (415 bytes → 9,567 bytes)
- **핵심 기능**: 진단 세션 전환 (Default ↔ Extended)
- **관련 UDS 서비스 ID**: 0x10
- **강의록 위치**: P.10-17 "Uds : Sessioncontrol"

---

## 1️⃣ 코드 비교: Complete → UDS_1

### 📝 [1] 전체 구조 비교

#### **Complete (기존 - 스텁)**
```c
#include "Rte_BswDcm.h"
#include "Definition.h"
#include "UdsNrcList.h"
#include "Utility.h"

void REcbBswDcm_initialize(void)
{
	(void) 0;  // ← 구현 없음
}

void REtBswDcm_bswDcmMain(void)
{
    (void) 0;  // ← 구현 없음
}

void REoiBswDcm_pDcmCmd_processDiagRequest(uint8 isPhysical, P_uint8 diagReq, uint8 length)
{
    (void) 0;  // ← 구현 없음
}

void REoiBswDcm_pDcmCmd_getDiagStatus(P_stDiagStatus pstDiagStatus)
{
    (void) 0;  // ← 구현 없음
}
```

#### **UDS_1 (새 코드 - 전체 구현)**
```c
#include "Rte_BswDcm.h"
#include "Definition.h"
#include "UdsNrcList.h"
#include "Utility.h"

//========================================
// ✅ NEW: 상수 정의 추가됨
//========================================
#define FRAME_TYPE_SINGLE 0u                    // CAN 프레임 타입
#define TIME_DIAG_P2_CAN_SERVER TIME_50MS       // 응답 타이밍: 50ms
#define TIME_DIAG_P2_MUL_CAN_SERVER TIME_5S    // 연장 타이밍: 5초
#define REF_VALUE_FILLER_BYTE 0xAAu             // 패딩 바이트

#define SID_DiagSessionControl 0x10u             // 서비스 ID
#define SFID_DiagnosticSessionControl_DefaultSession 0x01u
#define SFID_DiagnosticSessionControl_ExtendedSession 0x03u

//========================================
// ✅ NEW: 세션 상태 enum 추가됨
//========================================
enum {
	DIAG_SESSION_DEFAULT,    // 0: 기본 세션
	DIAG_SESSION_EXTENDED    // 1: 확장 세션
};

//========================================
// ✅ NEW: 데이터 구조체 추가됨 (핵심!)
//========================================
typedef struct {
	ST_DiagStatus stDiagStatus;              // 세션 상태 포함
    uint8 isServiceRequest;                  // 진단 요청 플래그
    uint8 isPhysical;                        // 물리 vs 기능 요청
    uint8 diagDataBufferRx[MAX_CAN_MSG_DLC]; // 수신 버퍼
    uint8 diagDataBufferTx[MAX_CAN_MSG_DLC]; // 송신 버퍼
    uint8 diagDataLengthRx;                  // 수신 데이터 길이
    uint8 diagDataLengthTx;                  // 송신 데이터 길이
}ST_BswDcmData;

static ST_BswDcmData stBswDcmData;           // ✅ 전역 진단 데이터

// ✅ NEW: 함수 미리 선언
static uint8 diagnosticSessionControl(void);

//========================================
// 초기화 함수 (변경 없음)
//========================================
void REcbBswDcm_initialize(void)
{
	(void) 0;
}

//========================================
// ✅ NEW: 진단 요청 처리 (완전 구현됨)
//========================================
void REtBswDcm_bswDcmMain(void)
{
	uint8 serviceId;
	uint8 errorResult = FALSE;
	
	// ✅ NEW: 진단 요청 확인
	if(stBswDcmData.isServiceRequest == TRUE)
	{
		stBswDcmData.isServiceRequest = FALSE;
		
		// ✅ NEW: 서비스 ID 추출 (첫 번째 바이트)
		serviceId = stBswDcmData.diagDataBufferRx[0];
		
		// ✅ NEW: 0x10 서비스만 처리 (향후 다른 서비스 추가 가능)
		if(serviceId == SID_DiagSessionControl)
		{
			errorResult = diagnosticSessionControl();
		}
		else
		{
			errorResult = NRC_ServiceNotSupported;
		}

		// ✅ NEW: 에러 응답 생성
		if(errorResult != FALSE)
		{
			stBswDcmData.diagDataLengthTx = 3u;
			stBswDcmData.diagDataBufferTx[0] = (FRAME_TYPE_SINGLE << 4u) | stBswDcmData.diagDataLengthTx;
			stBswDcmData.diagDataBufferTx[1] = 0x7Fu;  // 부정 응답
			stBswDcmData.diagDataBufferTx[2] = stBswDcmData.diagDataBufferRx[0];
			stBswDcmData.diagDataBufferTx[3] = errorResult;
		}

		// ✅ NEW: 응답 메시지 패딩 (0xAA로 채우기)
		for(uint8 i=stBswDcmData.diagDataLengthTx+1u; i<TX_MSG_LEN_PHY_RES; i++)
		{
			stBswDcmData.diagDataBufferTx[i] = REF_VALUE_FILLER_BYTE;
		}
		
		// ✅ NEW: CAN으로 응답 전송
		Rte_Call_BswDcm_rEcuAbsCan_canSend(TX_MSG_IDX_PHY_RES, stBswDcmData.diagDataBufferTx);
	}
}

//========================================
// ✅ NEW: 진단 요청 수신 처리
//========================================
void REoiBswDcm_pDcmCmd_processDiagRequest(uint8 isPhysical, P_uint8 diagReq, uint8 length)
{
	(void) length;
	
	// ✅ NEW: 프레임 타입 추출 (상위 4비트)
	uint8_t frameType = diagReq[0] >> 4u;
	
	if (frameType == FRAME_TYPE_SINGLE)
	{
		memset(stBswDcmData.diagDataBufferRx, 0, MAX_CAN_MSG_DLC);
		
		// ✅ NEW: 데이터 길이 추출 (하위 4비트)
		stBswDcmData.diagDataLengthRx = (diagReq[0] & 0x0Fu);
		
		// ✅ NEW: 물리/기능 요청 구분
		stBswDcmData.isPhysical = isPhysical;
		
		// ✅ NEW: 실제 데이터 복사 (diagReq[1]부터 시작)
		(void)memcpy(stBswDcmData.diagDataBufferRx, &diagReq[1], stBswDcmData.diagDataLengthRx);
		
		// ✅ NEW: 메인 루프에서 처리하도록 플래그 설정
		stBswDcmData.isServiceRequest = TRUE;
	}
}

//========================================
// ✅ NEW: 진단 상태 조회 (구현 미흡)
//========================================
void REoiBswDcm_pDcmCmd_getDiagStatus(P_stDiagStatus pstDiagStatus)
{
    (void) 0;  // TODO: 세션 상태 반환 구현 필요
}

//========================================
// ✅ [핵심 함수] 세션 제어 구현
//========================================
static uint8 diagnosticSessionControl(void)
{
	uint8 errorResult = FALSE;
	
	// ✅ NEW: 서브함수 ID 추출 (비트 7 제외)
	uint8 subFunctionId = stBswDcmData.diagDataBufferRx[1] & 0x7Fu; 0111 1111
	
	// ✅ NEW: 요청 메시지 길이 검증 (정확히 2바이트)
	if(stBswDcmData.diagDataLengthRx != 2u)
	{
		errorResult = NRC_IncorrectMessageLengthOrInvalidFormat;
	}
	// ✅ NEW: Default 세션 요청
	else if(subFunctionId == SFID_DiagnosticSessionControl_DefaultSession)
	{
		stBswDcmData.stDiagStatus.currentSession = DIAG_SESSION_DEFAULT;
		// ← 이제부터 기본 세션 모드
	}
	// ✅ NEW: Extended 세션 요청
	else if(subFunctionId == SFID_DiagnosticSessionControl_ExtendedSession)
	{
		stBswDcmData.stDiagStatus.currentSession = DIAG_SESSION_EXTENDED;
		// ← 이제부터 확장 세션 모드 (SecurityAccess, Write 가능)
	}
	// ✅ NEW: 지원 안 하는 서브함수
	else
	{
		errorResult = NRC_SubFunctionNotSupported;
	}
	
	// ✅ NEW: 성공 시 긍정 응답 구성
	if(errorResult == FALSE)
	{
		stBswDcmData.diagDataLengthTx = 6u;
		
		// 프레임 타입 + 길이
		stBswDcmData.diagDataBufferTx[0] = (FRAME_TYPE_SINGLE << 4u) | stBswDcmData.diagDataLengthTx;
		
		// 서비스 ID + 0x40 (긍정 응답 코드)
		stBswDcmData.diagDataBufferTx[1] = SID_DiagSessionControl + 0x40u;  // 0x50
		
		// 요청한 세션 ID 에코
		stBswDcmData.diagDataBufferTx[2] = subFunctionId;
		
		// ✅ NEW: P2 타이밍 전송 (50ms)
		stBswDcmData.diagDataBufferTx[3] = (TIME_DIAG_P2_CAN_SERVER & 0xFF00u) >> 8u;  // High
		stBswDcmData.diagDataBufferTx[4] = (TIME_DIAG_P2_CAN_SERVER & 0xFFu);          // Low
		
		// ✅ NEW: P2* 타이밍 전송 (5000ms)
		stBswDcmData.diagDataBufferTx[5] = (uint8)(((TIME_DIAG_P2_MUL_CAN_SERVER/10u) & 0xFF00u) >> 8u);  // High
		stBswDcmData.diagDataBufferTx[6] = ((TIME_DIAG_P2_MUL_CAN_SERVER/10u) & 0xFFu);                    // Low
	}
	
	return errorResult;  // 0 = 성공, 0x?? = 에러 코드
}
```

---

## 2️⃣ 참고 자료

### 📚 강의록 참고 (OTA_Bootloader.md)

#### **[1] 서비스 개요**
- **위치**: P.5-6 "UDS : What Is UDS?" + "US : Service List"
- **내용**: 
  - UDS는 ISO 14229 표준 기반 진단 프로토콜
  - DiagnosticSessionControl (0x10)은 모든 서비스의 **첫 번째** 게이트웨이
  - 각 서비스마다 세션별 지원 여부가 다름

#### **[2] 세션 규칙**
- **위치**: P.6 "Table 23 - Services allowed during default and non-default diagnostic session"
- **핵심 내용**:
  ```
  ┌──────────────────────────────┬─────────────┬──────────────────┐
  │ Service                      │ DefaultSess │ ExtendedSession  │
  ├──────────────────────────────┼─────────────┼──────────────────┤
  │ DiagnosticSessionControl 0x10│      ✅     │        ✅        │
  │ SecurityAccess 0x27          │      ❌     │        ✅        │ ← Extended 전용!
  │ WriteDataByIdentifier 0x2E   │      ❌     │        ✅        │ ← Extended 전용!
  │ ReadDataByIdentifier 0x22    │      ✅     │        ✅        │
  └──────────────────────────────┴─────────────┴──────────────────┘
  ```
  **의미**: UDS_1(세션 전환)을 먼저 실행해야 UDS_3(보안인증)과 UDS_7(쓰기)가 작동

#### **[3] 세션 타입 정의**
- **위치**: P.10-11 "Uds : Sessioncontrol"
- **이미지**: `10_image_0.png` (UDS 0x10 개요)
- **요청 메시지**:
  ```
  Byte #1: Service ID (0x10)
  Byte #2: Sub-function (0x01 = Default, 0x03 = Extended)
  Filler: 0x55 (5개 바이트)
  ```
- **응답 메시지**:
  ```
  Byte #1: 0x50 (0x10 + 0x40 = 긍정 응답)
  Byte #2: Sub-function (요청한 것과 동일)
  Byte #3-4: P2_Server_max (High/Low)
  Byte #5-6: P2*_Server_max (High/Low)
  Filler: 0xAA (1개 바이트)
  ```

#### **[4] 타이밍 파라미터**
- **위치**: P.11 "Table 28 - sessionParameterRecord definition"
- **이미지**: `11_image_0.png`
- **P2 타이밍** (50ms):
  - ECU가 정상 진단 요청에 응답해야 하는 최대 시간
  - 응답이 없으면 진단기는 타임아웃 처리
  
- **P2* 타이밍** (5000ms):
  - 플래싱, 프로그래밍 등 장시간 작업 중 ECU가 "잠깐만 기다려" 신호를 보낼 때
  - "Enhanced Timeout" 역할

#### **[5] 코드 구현 예시**
- **위치**: P.14-17 "Uds : Sessioncontrol Code / Uds_1"
- **이미지**: 
  - `15_image_0.png` (데이터 구조체 및 상수)
  - `16_image_0.png` (메인 루프 처리)
  - `17_image_0.png` (diagnosticSessionControl 함수)

#### **[6] 실제 통신 예시**
- **위치**: P.18 "Practice : Uds - Sessioncontrol"
- **이미지**: 
  - `18_image_0.png` (트레이스 32 로그)
  - `18_image_1.png` (Default → Extended 전환)
  - `18_image_2.png` (타이밍 분석)
- **로그 해석**:
  ```
  [TX] 7D3(8): 02 10 01 55 55 55 55  ← Default 세션 요청
  [RX] 7DB(8): 06 50 01 00 32 13 88  ← 응답 (P2=50ms, P2*=5000ms)
  
  [TX] 7D3(8): 02 10 03 55 55 55 55  ← Extended 세션 요청
  [RX] 7DB(8): 06 50 03 00 32 13 88  ← 응답 (동일)
  ```

#### **[7] 에러 코드 (NRC)**
- **위치**: P.19-21 "Uds : Nrc (Negative Response Codes)"
- **이미지**: `20_image_0.png`, `21_image_0.png`
- **관련 에러**:
  - `NRC_IncorrectMessageLengthOrInvalidFormat` (0x13): 메시지 길이 오류
  - `NRC_SubFunctionNotSupported` (0x12): 지원 안 하는 서브함수
  - `NRC_ServiceNotSupported` (0x11): 지원 안 하는 서비스

---

## 3️⃣ 핵심 개념 설명

### 🎯 세션 제어의 목적

```
Default Session          Extended Session
    ↑                           ↑
    │                           │
┌───┴──────────────────────────┴───┐
│  ECU 안전성 & 접근 제어           │
├──────────────────────────────────┤
│                                  │
│  기본 세션:                        │
│  - 진단 읽기만 가능                │
│  - 안전한 상태 유지                │
│  - 무제한 접속 가능                │
│                                  │
│  확장 세션:                        │
│  - Security Access(0x27) 후      │
│  - 플래싱, 쓰기, 프로그래밍 가능 │
│  - 타임아웃 관리 필수             │
│                                  │
└──────────────────────────────────┘
```

### 📋 실행 흐름도

```
진단기에서 요청
    ↓
[CAN RX] 0x02 0x10 0x03 ...
    ↓
REoiBswDcm_pDcmCmd_processDiagRequest()
├─ 프레임 타입 확인 (Single/Multi)
├─ 데이터 길이 추출
└─ stBswDcmData에 저장 + isServiceRequest = TRUE
    ↓
REtBswDcm_bswDcmMain()
├─ isServiceRequest == TRUE 확인
├─ serviceId = 0x10 (DiagnosticSessionControl)
└─ diagnosticSessionControl() 호출
    ↓
diagnosticSessionControl()
├─ subFunctionId = 0x03 (Extended)
├─ 길이 검증 (2바이트 정확)
├─ stDiagStatus.currentSession = DIAG_SESSION_EXTENDED
└─ 응답 메시지 구성
    ↓
응답 패딩 (0xAA로 채우기)
    ↓
[CAN TX] 0x06 0x50 0x03 0x00 0x32 ...
    ↓
진단기에서 응답 수신
    ↓
이제 Extended 세션에서만 가능한 서비스 실행 가능
예: UDS_3 (SecurityAccess), UDS_7 (WriteDataByIdentifier)
```

### 🔑 주요 구조체: ST_BswDcmData

```c
// 이 구조체가 진단의 모든 상태를 관리
typedef struct {
    ST_DiagStatus stDiagStatus;              // ✅ 현재 세션 상태 저장
    uint8 isServiceRequest;                  // 처리할 요청 있음?
    uint8 isPhysical;                        // 물리(TRUE) vs 기능(FALSE)
    uint8 diagDataBufferRx[MAX_CAN_MSG_DLC]; // [정씀] 받은 데이터
    uint8 diagDataBufferTx[MAX_CAN_MSG_DLC]; // [정씀] 보낼 데이터
    uint8 diagDataLengthRx;                  // 받은 데이터 길이
    uint8 diagDataLengthTx;                  // 보낼 데이터 길이
}ST_BswDcmData;
```

### 🚀 메시지 프로토콜 (ISO 15765)

#### **Single Frame 포맷**
```
Byte #0: [Frame Type (상위 4bit)|Length (하위 4bit)]
         예: 0x03 = SINGLE(0) + 길이3
Byte #1: Service ID (0x10)
Byte #2: Sub-function (0x01 or 0x03)
Byte #3~7: Filler (0x55/0xAA)
```

#### **이 구조를 코드로 구성하는 부분**
```c
// ✅ 요청 수신: REoiBswDcm_pDcmCmd_processDiagRequest()
frameType = diagReq[0] >> 4u;           // 상위 4비트 추출
dataLength = diagReq[0] & 0x0Fu;        // 하위 4비트 추출
memcpy(rxBuffer, &diagReq[1], dataLength); // Byte #1부터 실제 데이터

// ✅ 응답 전송: diagnosticSessionControl()
txBuffer[0] = (FRAME_TYPE_SINGLE << 4u) | 6;  // [0|6]
txBuffer[1] = 0x50;                            // 긍정 응답
txBuffer[2] = 0x03;                            // Sub-function
// ... 타이밍 정보
// ... Filler로 채우기
Rte_Call_BswDcm_rEcuAbsCan_canSend(...);
```

---

## 4️⃣ Complete vs UDS_1 변화 요약

| 항목 | Complete | UDS_1 | 이유 |
|------|----------|-------|------|
| **전체 코드량** | 415 bytes | 9,567 bytes | 세션 제어 로직 완전 구현 |
| **상태 관리** | 없음 | `ST_BswDcmData` 구조체 | 진단 상태 추적 필요 |
| **세션 상태** | - | `currentSession` (0/1) | Default ↔ Extended 전환 |
| **프레임 파싱** | - | `REoiBswDcm_pDcmCmd_processDiagRequest()` | CAN 메시지 분석 |
| **서비스 처리** | - | `diagnosticSessionControl()` | 0x10 서비스 구현 |
| **응답 메시지** | - | P2/P2* 타이밍 포함 | ISO 15765 표준 준수 |
| **에러 처리** | - | NRC 코드 반환 | 진단기에 오류 전달 |

---

## ✅ 결론

**UDS_1이 Complete와 다른 이유:**

1. **기능의 추가**: 기본 프레임워크 → 실제 동작하는 세션 제어 시스템
2. **상태 관리**: 진단 시스템의 상태를 추적할 데이터 구조 필요
3. **표준 준수**: ISO 14229 (UDS), ISO 15765 (CAN TP) 표준 따르기
4. **보안 게이트웨이**: 이후 서비스들(SecurityAccess, WriteData 등)의 전조건

**다음 단계 (UDS_2):**
- ECUeset (0x11): 하드웨어 리셋 제어
- SecurityAccess(0x27) 실행 전 필수
