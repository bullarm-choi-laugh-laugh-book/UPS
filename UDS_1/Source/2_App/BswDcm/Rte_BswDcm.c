#include "Rte_BswDcm.h"
#include "Definition.h"
#include "UdsNrcList.h"
#include "Utility.h"

//========================================
// 📌 상수 정의
//========================================
#define FRAME_TYPE_SINGLE 0u				// 🔹 CAN 프레임 타입: SINGLE (한번에 보내는 프레임)
#define TIME_DIAG_P2_CAN_SERVER		TIME_50MS		// ⏱️ P2 타이밍: 진단기 → ECU 응답 시간 (50ms)
#define TIME_DIAG_P2_MUL_CAN_SERVER	TIME_5S			// ⏱️ P2* 타이밍: 긴 작업용 (5초)
#define REF_VALUE_FILLER_BYTE		0xAAu			// 🔲 패딩 바이트 (메시지 뒷부분 채우기)

//========================================
// 🚀 UDS 서비스 ID
//========================================
#define SID_DiagSessionControl				0x10u		// 📋 서비스 0x10: 세션 제어

//========================================
// 🎯 세션 제어 서브 함수 ID (ServiceID 0x10의 상세 종류)
//========================================
#define SFID_DiagnosticSessionControl_DefaultSession		0x01u	// 기본 세션
#define SFID_DiagnosticSessionControl_ExtendedSession		0x03u	// 확장 세션

//========================================
// 🔀 세션 상태 열거형 (enum) - 현재 세션 모드
//========================================
/* DIAGNOSTIC_SESSION */
enum
{
	DIAG_SESSION_DEFAULT,		// 0️⃣ 기본 모드: 정상 운영
	DIAG_SESSION_EXTENDED		// 1️⃣ 확장 모드: 개발/테스트/캘리브레이션
};

//========================================
// 💾 [저장소] 진단 관련 전체 데이터
//========================================
typedef struct {
	ST_DiagStatus stDiagStatus;				// 📊 진단 상태 (현재 세션 모드 포함)
    uint8 isServiceRequest;					// 🚨 "처리할 진단 요청 있어?" 플래그
    uint8 isPhysical;						// 📌 물리 요청(TRUE) vs 기능 요청(FALSE)
    uint8 diagDataBufferRx[MAX_CAN_MSG_DLC];	// 📨 받은 진단 요청 데이터
    uint8 diagDataBufferTx[MAX_CAN_MSG_DLC];	// 📤 보낼 진단 응답 데이터
    uint8 diagDataLengthRx;					// 📏 받은 데이터의 길이
    uint8 diagDataLengthTx;					// 📏 보낼 데이터의 길이
}ST_BswDcmData;

static ST_BswDcmData stBswDcmData;  // ← 진단 데이터를 여기에 저장

// 📌 함수 미리 선언 (prototype)
static uint8 diagnosticSessionControl(void);

//========================================
// ⚙️ [초기화] 프로그램 시작 시 실행
//========================================
void REcbBswDcm_initialize(void)
{
	(void) 0;  // 현재는 특별히 할 일 없음 (필요하면 여기에 추가)
}

//========================================
// 🔄 [메인 루프] 계속 반복 실행
// 역할: BswCom에서 받은 진단 요청을 처리하고 응답하기
//========================================
void REtBswDcm_bswDcmMain(void)
{
	uint8 serviceId;				// 📋 받은 서비스 ID (예: 0x10 = 세션 제어)
	uint8 errorResult = FALSE;		// ❌ 에러 코드 저장 (FALSE=성공, 0x??=에러)
	
	// ❓ "처리할 진단 요청이 있어?" 확인
	if(stBswDcmData.isServiceRequest == TRUE)
	{
		// ✅ 플래그 초기화 (다음 요청까지 기다림)
		stBswDcmData.isServiceRequest = FALSE;
		
		// 📌 서비스 ID 추출 (첫번째 바이트)
		serviceId = stBswDcmData.diagDataBufferRx[0];  // 예: 0x10
		
		// 🎯 어떤 서비스인지 확인하고 처리
		if(serviceId == SID_DiagSessionControl)  // ← 0x10 서비스?
		{
			errorResult = diagnosticSessionControl();  // ✓ 세션 제어 함수 호출
		}
		else  // ← 지원하지 않는 서비스?
		{
			errorResult = NRC_ServiceNotSupported;  // ✗ 에러: 미지원 서비스
		}

		// 📤 응답 메시지 생성 및 전송
		if(errorResult != FALSE)  // ← 에러 발생했으면 에러 응답 보내기
		{
			// 에러 응답 메시지 구성:
			// [프레임정보|길이] [0x7F] [요청서비스ID] [에러코드]
			stBswDcmData.diagDataLengthTx = 3u;
			stBswDcmData.diagDataBufferTx[0] = (FRAME_TYPE_SINGLE << 4u) | stBswDcmData.diagDataLengthTx;
			// 예: 0x03 (SINGLE 프레임, 길이 3)
			
			stBswDcmData.diagDataBufferTx[1] = 0x7Fu;  // 🚨 부정 응답 코드 (서비스ID + 0x40)
			stBswDcmData.diagDataBufferTx[2] = stBswDcmData.diagDataBufferRx[0];  // 요청한 서비스 ID
			stBswDcmData.diagDataBufferTx[3] = errorResult;  // 에러 코드
		}

		// 🔲 응답 메시지 뒷부분 패딩 (0xAA로 채우기)
		for(uint8 i=stBswDcmData.diagDataLengthTx+1u; i<TX_MSG_LEN_PHY_RES; i++)
		{
			stBswDcmData.diagDataBufferTx[i] = REF_VALUE_FILLER_BYTE;
		}
		
		// 📡 응답 메시지를 CAN으로 전송
		Rte_Call_BswDcm_rEcuAbsCan_canSend(TX_MSG_IDX_PHY_RES, stBswDcmData.diagDataBufferTx);
	}
}

//========================================
// 🚨 [진단 요청 수신] BswCom에서 호출
// 역할: 진단 요청을 받아서 저장 및 처리 플래그 설정
//========================================
void REoiBswDcm_pDcmCmd_processDiagRequest(uint8 isPhysical, P_uint8 diagReq, uint8 length)
{
	(void) length;  // length 매개변수는 사용 안 함
	
	// 📌 프레임 타입 추출 (첫 바이트의 상위 4비트)
	// CAN 진단 메시지 형식: [FRAME_TYPE|LENGTH] [SID] [DATA...]
	uint8_t frameType = diagReq[0] >> 4u;  // 상위 4비트 추출
	
	// ✅ SINGLE 프레임이면 처리 (한번에 보내는 메시지)
	if (frameType == FRAME_TYPE_SINGLE)
	{
		// 🧹 버퍼 초기화
		memset(stBswDcmData.diagDataBufferRx, 0, MAX_CAN_MSG_DLC);
		
		// 📏 실제 데이터 길이 추출 (첫 바이트의 하위 4비트)
		stBswDcmData.diagDataLengthRx = (diagReq[0] & 0x0Fu);  // 하위 4비트 추출
		
		// 📌 물리 요청(TRUE) vs 기능 요청(FALSE) 구분
		stBswDcmData.isPhysical = isPhysical;
		
		// 📋 실제 데이터 복사 (서비스 ID, 서브함수 ID 등)
		// diagReq[0] = [FRAME_TYPE|LENGTH] 이므로
		// 실제 데이터는 diagReq[1]부터 시작
		(void)memcpy(stBswDcmData.diagDataBufferRx, &diagReq[1], stBswDcmData.diagDataLengthRx);
		
		// 🚀 메인 루프에서 처리하도록 플래그 세팅
		stBswDcmData.isServiceRequest = TRUE;
	}
}

//========================================
// 📊 [진단 상태 조회] 다른 모듈에서 현재 세션 상태 확인
// 역할: 현재 세션이 DEFAULT인지 EXTENDED인지 조회
//========================================
void REoiBswDcm_pDcmCmd_getDiagStatus(P_stDiagStatus pstDiagStatus)
{
    (void) 0;  // 현재는 구현 안 됨
    // ⚠️ TODO: 현재 세션 상태를 pstDiagStatus에 복사하는 로직 추가 필요
}

//========================================
// 🎯 [세션 제어] UDS 서비스 0x10 처리 (핵심 함수!)
// Input: diagDataBufferRx에 저장된 요청 (예: [0x10, 0x03])
// Output: 세션 변경 + 응답 메시지 생성
//========================================
static uint8 diagnosticSessionControl(void)
{
	uint8 errorResult = FALSE;  // ✅ 성공 여부 저장
	
	// 📌 서브함수 ID 추출 (두번째 바이트)
	// diagDataBufferRx[0] = 서비스 ID (0x10)
	// diagDataBufferRx[1] = 서브함수 ID (0x01 or 0x03)
	uint8 subFunctionId = stBswDcmData.diagDataBufferRx[1] & 0x7Fu;
	// & 0x7F는 비트 7 마스킹 (UDS 규칙: 비트7은 무시)
	
	// 🔍 요청 데이터 길이 검증
	if(stBswDcmData.diagDataLengthRx != 2u)  // 정확히 2바이트여야 함
	{
		// [0x10] [0x01 or 0x03] = 2바이트
		errorResult = NRC_IncorrectMessageLengthOrInvalidFormat;  // ❌ 길이 에러
	}
	// ✅ Default 세션 요청?
	else if(subFunctionId == SFID_DiagnosticSessionControl_DefaultSession)  // 0x01
	{
		stBswDcmData.stDiagStatus.currentSession = DIAG_SESSION_DEFAULT;
		// currentSession = 0 (기본 모드)
		// 이제 시스템은 기본 모드로 동작
	}
	// ✅ Extended 세션 요청?
	else if(subFunctionId == SFID_DiagnosticSessionControl_ExtendedSession)  // 0x03
	{
		stBswDcmData.stDiagStatus.currentSession = DIAG_SESSION_EXTENDED;
		// currentSession = 1 (확장 모드)
		// 이제 시스템은 확장 모드로 동작
	}
	// ❌ 지원하지 않는 세션?
	else
	{
		errorResult = NRC_SubFunctionNotSupported;  // ❌ 미지원 서브함수
	}
	
	// 📤 성공 시 긍정 응답 생성
	if(errorResult == FALSE)
	{
		// 응답 메시지 포맷:
		// [프레임] [서비스ID+0x40] [서브함수ID] [P2 High] [P2 Low] [P2* High] [P2* Low]
		
		stBswDcmData.diagDataLengthTx = 6u;  // 총 길이 6바이트
		
		stBswDcmData.diagDataBufferTx[0] = (FRAME_TYPE_SINGLE << 4u) | stBswDcmData.diagDataLengthTx;
		// 예: 0x06 (SINGLE 프레임, 길이 6)
		
		stBswDcmData.diagDataBufferTx[1] = SID_DiagSessionControl + 0x40u;
		// 0x10 + 0x40 = 0x50 (긍정 응답)
		
		
		stBswDcmData.diagDataBufferTx[2] = subFunctionId;
		// 0x01 또는 0x03 (요청한 세션 ID 에코)
		
		// ⏱️ P2 타이밍 (진단기 → ECU 응답 시간)
		stBswDcmData.diagDataBufferTx[3] = (TIME_DIAG_P2_CAN_SERVER & 0xFF00u) >> 8u;  // High Byte
		stBswDcmData.diagDataBufferTx[4] = (TIME_DIAG_P2_CAN_SERVER & 0xFFu);			// Low Byte
		
		// ⏱️ P2* 타이밍 (긴 작업용 확장 시간)
		stBswDcmData.diagDataBufferTx[5] = (uint8)(((TIME_DIAG_P2_MUL_CAN_SERVER/10u) & 0xFF00u) >> 8u);  // High Byte
		stBswDcmData.diagDataBufferTx[6] = ((TIME_DIAG_P2_MUL_CAN_SERVER/10u) & 0xFFu);					  // Low Byte
	}
	
	return errorResult;  // 결과 반환 (0=성공, 0x??=에러코드)
}
