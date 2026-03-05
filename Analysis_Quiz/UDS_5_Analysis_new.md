# UDS_5 분석: CommunicationControl (0x28)

## 📌 개요
- **변경 파일**: `Rte_BswDcm.c` (UDS_4 → UDS_5)
- **핵심 기능**: OTA 중 CAN 통신 능/비활성화 제어
- **관련 UDS 서비스 ID**: 0x28
- **강의록 위치**: P.XX "CommunicationControl" (확인 예정)

---

## 1️⃣ 코드 비교: UDS_4 → UDS_5

### 📝 [1] SID 및 SubFunction 정의 추가

#### **UDS_4 (기존 - CommunicationControl 없음)**
```c
#define SID_DiagSessionControl				0x10u
#define SID_EcuReset						0x11u
#define SID_SecurityAccess					0x27u

#define SFID_DiagnosticSessionControl_DefaultSession					0x01u
#define SFID_DiagnosticSessionControl_ExtendedSession					0x03u
#define SFID_EcuReset_HardReset 										0x01u
#define SFID_SecurityAccess_RequestSeed									0x01u
#define SFID_SecurityAccess_SendKey										0x02u
```

#### **UDS_5 (새 코드 - CommunicationControl 추가)**
```c
#define SID_DiagSessionControl				0x10u
#define SID_EcuReset						0x11u
#define SID_SecurityAccess					0x27u
#define SID_CommunicationControl			0x28u  // ✅ NEW

#define SFID_DiagnosticSessionControl_DefaultSession					0x01u
#define SFID_DiagnosticSessionControl_ExtendedSession					0x03u
#define SFID_EcuReset_HardReset 										0x01u
#define SFID_SecurityAccess_RequestSeed									0x01u
#define SFID_SecurityAccess_SendKey										0x02u
#define SFID_CommunicationControl_enableRxAndTx							0x00u   // ✅ NEW
#define SFID_CommunicationControl_disableRxAndTx						0x03u   // ✅ NEW

#define CommunicationType_normalCommunicationMessages	0x01u  // ✅ NEW
```

---

### 📝 [2] 함수 선언 추가

#### **UDS_4 (기존)**
```c
static uint8 diagnosticSessionControl(void);
static uint8 ecuReset(void);
static uint8 securityAccess(void);
```

#### **UDS_5 (새 코드 - communicationControl() 함수 선언 추가)**
```c
static uint8 diagnosticSessionControl(void);
static uint8 ecuReset(void);
static uint8 securityAccess(void);
static uint8 communicationControl(void);  // ✅ NEW 함수 선언
```

---

### 📝 [3] REtBswDcm_bswDcmMain() 함수 - 서비스 라우팅 확장

#### **UDS_4 (기존 - 3개 서비스 처리)**
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
		else if(serviceId == SID_SecurityAccess)
		{
			errorResult = securityAccess();
		}
		else
		{
			errorResult = NRC_ServiceNotSupported;
		}
		// ... 응답 송신 부분
	}
}
```

#### **UDS_5 (새 코드 - CommunicationControl 라우팅 추가)**
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
		else if(serviceId == SID_SecurityAccess)
		{
			errorResult = securityAccess();
		}
		else if(serviceId == SID_CommunicationControl)  // ✅ NEW 라우팅 추가
		{
			errorResult = communicationControl();
		}
		else
		{
			errorResult = NRC_ServiceNotSupported;
		}
		// ... 응답 송신 부분
	}
}
```

---

### 📝 [4] communicationControl() 함수 - 완전히 새로운 구현

#### **UDS_4 (없음)**
```c
// communicationControl() 함수 없음
```

#### **UDS_5 (새 코드 - 완전히 새로운 함수 추가)**
```c
static uint8 communicationControl(void)
{
	uint8 subFunctionId = stBswDcmData.diagDataBufferRx[1] & 0x7Fu;
	uint8 errorResult = FALSE;
	uint8 communicationType = stBswDcmData.diagDataBufferRx[2];

	//========================================
	// ✅ 검증 단계 1: 메시지 길이
	//========================================
	if(stBswDcmData.diagDataLengthRx != 3u)
	{
		errorResult = NRC_IncorrectMessageLengthOrInvalidFormat;
	}
	//========================================
	// ✅ 검증 단계 2: 세션 (ExtendedSession만 허용)
	//========================================
	else if(stBswDcmData.stDiagStatus.currentSession != DIAG_SESSION_EXTENDED)
	{
		errorResult = NRC_SubFunctionNotSupportedInActiveSession;
	}
	//========================================
	// ✅ 검증 단계 3: Communication Type
	//========================================
	else if(communicationType != CommunicationType_normalCommunicationMessages)
	{
		errorResult = NRC_RequestOutRange;
	}
	//========================================
	// ✅ 실행 단계: enableRxAndTx (SubFunc 0x00)
	//========================================
	else if(subFunctionId == SFID_CommunicationControl_enableRxAndTx)
	{
		stBswDcmData.stDiagStatus.isCommunicationDisable = FALSE;  // 통신 활성화!
		
		// 성공 응답 구성
		stBswDcmData.diagDataLengthTx = 2u;
		stBswDcmData.diagDataBufferTx[0] = (FRAME_TYPE_SINGLE << 4u) | stBswDcmData.diagDataLengthTx;
		stBswDcmData.diagDataBufferTx[1] = SID_CommunicationControl + 0x40u;  // 0x68
		stBswDcmData.diagDataBufferTx[2] = subFunctionId;  // Echo 0x00
	}
	//========================================
	// ✅ 실행 단계: disableRxAndTx (SubFunc 0x03)
	//========================================
	else if(subFunctionId == SFID_CommunicationControl_disableRxAndTx)
	{
		stBswDcmData.stDiagStatus.isCommunicationDisable = TRUE;   // 통신 비활성화!
		
		// 성공 응답 구성
		stBswDcmData.diagDataLengthTx = 2u;
		stBswDcmData.diagDataBufferTx[0] = (FRAME_TYPE_SINGLE << 4u) | stBswDcmData.diagDataLengthTx;
		stBswDcmData.diagDataBufferTx[1] = SID_CommunicationControl + 0x40u;  // 0x68
		stBswDcmData.diagDataBufferTx[2] = subFunctionId;  // Echo 0x03
	}
	//========================================
	// ✅ 에러: 지원 안 하는 SubFunc
	//========================================
	else
	{
		errorResult = NRC_SubFunctionNotSupported;
	}
	
	return errorResult;
}
```

---

## 2️⃣ 함수 상세 분석

### communicationControl() 핵심 로직

#### **요청/응답 메시지 포맷**
```
[요청]
Byte 0: [FrameType:0 | Length:3]
Byte 1: SID 0x28
Byte 2: SubFunc (0x00=Enable, 0x03=Disable)
Byte 3: CommType (0x01=Normal)

[응답]
Byte 0: [FrameType:0 | Length:2]
Byte 1: SID+0x40 = 0x68
Byte 2: SubFunc Echo
```

#### **SubFunction 종류**
| SubFunc | 값 | 의미 | 결과 상태 |
|---------|-----|------|----------|
| enableRxAndTx | 0x00 | 통신 활성화 | isCommunicationDisable = FALSE |
| disableRxAndTx | 0x03 | 통신 비활성화 | isCommunicationDisable = TRUE |

---

## 3️⃣ 프로토콜 동작 및 검증 순서

### 검증 체계 (3단계)

```
요청 도착
    ↓
[검증 1] 길이 = 3바이트?  NO → NRC_IncorrectMessageLengthOrInvalidFormat
    ↓ YES
[검증 2] 세션 = Extended?  NO → NRC_SubFunctionNotSupportedInActiveSession
    ↓ YES
[검증 3] CommType = 0x01?  NO → NRC_RequestOutRange
    ↓ YES
[실행] SubFunc별 처리
    ↓ SubFunc = 0x00 → isCommunicationDisable = FALSE
    ↓ SubFunc = 0x03 → isCommunicationDisable = TRUE
    ↓
응답 송신 (길이:2)
```

### OTA 흐름에서의 역할

```
[1단계] DiagSessionControl (0x10) → ExtendedSession 전환
[2단계] SecurityAccess (0x27) → ECU 인증
[3단계] CommunicationControl (0x28, Disable) ← ✅ 통신 OFF
        ↓
        Flash 메모리 쓰기 시작 (CAN 메시지 차단)
        ↓
[4단계] CommunicationControl (0x28, Enable) ← ✅ 통신 ON
[5단계] EcuReset (0x11) → 재부팅
```

**왜 필요한가?**
- Flash 프로그래밍 중에 CAN 인터럽트 발생 → 작업 중단 위험
- 미리 통신을 &quot;우아하게(gracefully) 비활성화&quot; → 예측 불가능한 메시지 차단

---

## 4️⃣ 에러 처리 분석

| 에러 코드 | 상황 | 원인 | 예시 |
|---------|------|------|------|
| NRC_IncorrectMessageLengthOrInvalidFormat | 길이 ≠ 3 | 프로토콜 오류 | [02][28][00] (2바이트만 전송) |
| NRC_SubFunctionNotSupportedInActiveSession | 세션 ≠ Extended | 권한 부족 | DefaultSession에서 요청 |
| NRC_RequestOutRange | CommType ≠ 0x01 | 지원 안 함 | CommType = 0xFF |
| NRC_SubFunctionNotSupported | SubFunc ≠ 0x00, 0x03 | 미지원 | SubFunc = 0x05 |

---

## 5️⃣ UDS_4 vs UDS_5 비교

| 항목 | UDS_4 (SecurityAccess 0x27) | UDS_5 (CommunicationControl 0x28) |
|------|---|---|
| **서비스 목적** | 인증 (Seed↔Key) | **통신 제어** |
| **서비스 ID** | 0x27 | **0x28** |
| **SubFunction** | 0x01(Seed), 0x02(Key) | **0x00(Enable), 0x03(Disable)** |
| **요청 길이** | 2바이트 | **3바이트** |
| **응답 길이** | 4바이트 | **2바이트** |
| **필수 세션** | ExtendedSession | ExtendedSession |
| **추가 파라미터** | 없음 | **CommType (1바이트)** |
| **핵심 상태 플래그** | isSecurityUnlock | **isCommunicationDisable** |

---

## 📌 핵심 포인트

1. **3바이트 포맷**: 기존 서비스와 달리 **SID + SubFunc + CommType**
2. **3단계 검증**: 길이 → 세션 → CommType 순차 검증
3. **상태 플래그**: `isCommunicationDisable` 플래그로 실제 통신 제어
4. **응답 구조**: Enable/Disable 모두 **길이:2, SID+0x40, SubFunc Echo**
5. **OTA 필수**: Flash 작업 중 **예측 불가능한 메시지 차단**
6. **세션 의존성**: ExtendedSession이 아니면 **항상 실패**
