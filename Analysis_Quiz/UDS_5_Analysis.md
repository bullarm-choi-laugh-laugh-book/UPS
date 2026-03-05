# UDS_5 (CommunicationControl 0x28) 분석

> **UDS_5의 목적**: OTA 과정에서 ECU 내부 작업 중 CAN 통신 제어!
> Flash burn 중에는 통신을 꺼서 간섭 방지, 완료 후 다시 킨다 = OTA의 필수 기능 📡

---

## 1️⃣ 코드 위치 및 SID 정의

### UDS_5에서 추가된 SID

```c
#define SID_DiagSessionControl				0x10u
#define SID_EcuReset						0x11u
#define SID_SecurityAccess					0x27u
#define SID_CommunicationControl			0x28u  // ✅ NEW: UDS_5!

#define SFID_CommunicationControl_enableRxAndTx		0x00u   // Enable
#define SFID_CommunicationControl_disableRxAndTx	0x03u   // Disable

#define CommunicationType_normalCommunicationMessages	0x01u
```

---

## 2️⃣ communicationControl() 함수 - 핵심 로직

### 요청 포맷

```
Request:  [길이:3][SID:0x28][SubFunc][CommType]
Response: [길이:2][SID+0x40:0x68][SubFunc]
```

### 함수 구현

```c
static uint8 communicationControl(void)
{
	uint8 subFunctionId = stBswDcmData.diagDataBufferRx[1] & 0x7Fu;  // [1] 바이트에서 subfunc 추출
	uint8 errorResult = FALSE;
	uint8 communicationType = stBswDcmData.diagDataBufferRx[2];       // [2] 바이트: Communication Type

	// ✅ 1단계: 메시지 길이 검증
	if(stBswDcmData.diagDataLengthRx != 3u)  // 정확히 3바이트여야 함
	{
		errorResult = NRC_IncorrectMessageLengthOrInvalidFormat;
	}
	// ✅ 2단계: 세션 검증 (EXTENDED만 허용)
	else if(stBswDcmData.stDiagStatus.currentSession != DIAG_SESSION_EXTENDED)
	{
		errorResult = NRC_SubFunctionNotSupportedInActiveSession;
	}
	// ✅ 3단계: Communication Type 검증
	else if(communicationType != CommunicationType_normalCommunicationMessages)
	{
		errorResult = NRC_RequestOutRange;
	}
	// ✅ 4단계: enableRxAndTx 처리 (Subfunction 0x00)
	else if(subFunctionId == SFID_CommunicationControl_enableRxAndTx)
	{
		stBswDcmData.stDiagStatus.isCommunicationDisable = FALSE;  // ✅ 통신 활성화!
		stBswDcmData.diagDataLengthTx = 2u;
		stBswDcmData.diagDataBufferTx[0] = (FRAME_TYPE_SINGLE << 4u) | stBswDcmData.diagDataLengthTx;
		stBswDcmData.diagDataBufferTx[1] = SID_CommunicationControl + 0x40u;  // 0x68
		stBswDcmData.diagDataBufferTx[2] = subFunctionId;           // Echo 0x00
	}
	// ✅ 5단계: disableRxAndTx 처리 (Subfunction 0x03)
	else if(subFunctionId == SFID_CommunicationControl_disableRxAndTx)
	{
		stBswDcmData.stDiagStatus.isCommunicationDisable = TRUE;   // ✅ 통신 비활성화!
		stBswDcmData.diagDataLengthTx = 2u;
		stBswDcmData.diagDataBufferTx[0] = (FRAME_TYPE_SINGLE << 4u) | stBswDcmData.diagDataLengthTx;
		stBswDcmData.diagDataBufferTx[1] = SID_CommunicationControl + 0x40u;  // 0x68
		stBswDcmData.diagDataBufferTx[2] = subFunctionId;           // Echo 0x03
	}
	else
	{
		errorResult = NRC_SubFunctionNotSupported;
	}
	return errorResult;
}
```

---

## 3️⃣ 주요 변화: UDS_4 vs UDS_5

| 항목 | UDS_4 (SecurityAccess 0x27) | UDS_5 (CommunicationControl 0x28) |
|------|---------------------------|-------------------------------------|
| **목적** | 인증 (Seed↔Key) | 통신 제어 (활성화/비활성화) |
| **Subfunctions** | 0x01 (Seed), 0x02 (Key) | 0x00 (Enable), 0x03 (Disable) |
| **필요 세션** | ExtendedSession | ExtendedSession |
| **요청 길이** | 2바이트 (HardReset처럼) | **3바이트** (새로운 파라미터!) |
| **응답 길이** | 4바이트 (Seed 포함) | 2바이트 (심플) |
| **핵심 상태** | isSecurityUnlock | **isCommunicationDisable** |
| **요청 포맷** | [SID][SubFunc] | [SID][SubFunc][CommType] |
| **응답 포맷** | [SID+0x40][SubFunc][Seed_Hi][Seed_Lo] | [SID+0x40][SubFunc] |

---

## 4️⃣ OTA 흐름에서 UDS_5의 역할

```
[OTA 과정]
① DiagSessionControl (0x10) → ExtendedSession으로 전환
② SecurityAccess (0x27) → ECU 인증 (Seed/Key)
③ CommunicationControl (0x28, Disable) → ✅ 통신 OFF → Flash 작업 중 간섭 차단!
④ (ROM 플래싱 진행)
⑤ CommunicationControl (0x28, Enable) → ✅ 통신 ON → 플래싱 완료, 다시 통신 시작!
⑥ EcuReset (0x11) → 새 펌웨어로 재부팅
```

**왜 필요한가?**
- Flash 쓰기 중 CAN 메시지가 들어오면 ECU가 busy state로 빠짐
- 따라서 Flash 작업 전에 **미리 통신을 꺼야 함** → 예측 불가능한 버스 트래픽 차단

---

## 5️⃣ 에러 처리 (Error Code)

| 에러 번호 | 상황 | 예시 |
|---------|------|------|
| `NRC_IncorrectMessageLengthOrInvalidFormat` | 길이 != 3 | Request가 [03][28][00]이 아님 |
| `NRC_SubFunctionNotSupportedInActiveSession` | Session != Extended | DefaultSession에서 요청 |
| `NRC_RequestOutRange` | CommType != 0x01 | CommType이 0xFF 같은 잘못된 값 |
| `NRC_SubFunctionNotSupported` | SubFunc != 0x00, 0x03 | SubFunc가 0x05 같은 미지원 값 |

---

## 6️⃣ POST-OTA 검증 (테스트 관점)

| 단계 | 검증 항목 | 확인 포인트 |
|------|---------|-----------|
| **OTA 시작** | isCommunicationDisable = FALSE | 초기값은 통신 활성 |
| **Flash 전** | CommunicationControl Disable 전송 | isCommunicationDisable = TRUE |
| **Flash 중** | CAN 메시지 수신 여부 | 메시지 도착하면 버림 |
| **Flash 후** | CommunicationControl Enable 전송 | isCommunicationDisable = FALSE |
| **재부팅 후** | 일반 CAN 통신 정상화 | ECU가 다시 메시지 수신 |

---

## 📌 핵심 포인트

1. **세션 의존성**: ExtendedSession이 아니면 CommunicationControl 불가
2. **3바이트 포맷**: UDS_1, UDS_2, UDS_4와 달리 요청이 **3바이트 + 파라미터**
3. **상태 플래그**: `isCommunicationDisable` 플래그가 ECU 메인 루프에서 CAN 수신 차단
4. **OTA 필수 기능**: Flash burn 중 통신 간섭 방지 → 펌웨어 무결성 보호
5. **응답 구조**: Enable/Disable 모두 동일한 응답 구조 (단지 SubFunc만 Echo)
