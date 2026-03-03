#include "Rte_BswCom.h"
#include "Definition.h"
#include "Utility.h"

//========================================
// 🔵 [저장소 1] CAN 메시지 버퍼 - 모든 메시지를 여기에 저장
//========================================
typedef struct {
    uint8 canMsgBuffer[TOTAL_CAN_MSG_COUNT][MAX_CAN_MSG_DLC];
} ST_BswComData;
static ST_BswComData stBswComData; //얘가 저장소

//========================================
// 🟢 [저장소 2] 센서에서 받은 데이터 - 스티어링 센서값 저장
//========================================
typedef struct
{
	uint8 message[RX_MSG_LEN_SAS_01];			// 📨 원본 바이너리 메시지 (예: 01 2F A5...)
	uint8 sas01TestValue01;						// 📊 추출된 값1 (예: 45도)
	uint16 sas01TestValue02;					// 📊 추출된 값2 (예: 온도 23도)
	uint16 sas01TestValue03;					// 📊 추출된 값3 (예: 상태 플래그)
}ST_Sas01;
static ST_Sas01 stSas01;

//========================================
// 🔴 [저장소 3] 보낼 응답 데이터 - "받았습니다" 메시지 저장
//========================================
typedef struct
{
	uint8 message[TX_MSG_LEN_SDC_01];			// 📨 보낼 원본 바이너리 메시지
	uint8 aliveCount;							// ✅ "살아있어요" 신호 (1,2,3,4,...)
	uint16 echoSas01TestValue02;				// 🔄 센서값 복사본 1 (에코)
	uint16 echoSas01TestValue03;				// 🔄 센서값 복사본 2 (에코)
}ST_Sdc01;
static ST_Sdc01 stSdc01;

// 📌 함수들의 미리 선언 (prototype)
static void processingDataRx(void);
static void processingDataTx(void);
static void sendCanTx(void);

//========================================
// ⚙️ [1단계] 초기화 함수 - 프로그램 시작할 때 실행
// "어떤 메시지 받을거야?" 미리 등록하기
//========================================
void REcbBswCom_initialize(void)
{
	Rte_Call_BswCom_rEcuAbsCan_setPduInfo(RX_MSG_IDX_PHY_REQ, RX_MSG_LEN_PHY_REQ, RX_MSG_ID_PHY_REQ, stBswComData.canMsgBuffer[RX_MSG_IDX_PHY_REQ]);
	Rte_Call_BswCom_rEcuAbsCan_setPduInfo(RX_MSG_IDX_FUN_REQ, RX_MSG_LEN_FUN_REQ, RX_MSG_ID_FUN_REQ, stBswComData.canMsgBuffer[RX_MSG_IDX_FUN_REQ]);
    Rte_Call_BswCom_rEcuAbsCan_setPduInfo(RX_MSG_IDX_CCP_REQ, RX_MSG_LEN_CCP_REQ, RX_MSG_ID_CCP_REQ, stBswComData.canMsgBuffer[RX_MSG_IDX_CCP_REQ]);
    Rte_Call_BswCom_rEcuAbsCan_setPduInfo(RX_MSG_IDX_SAS_01, RX_MSG_LEN_SAS_01, RX_MSG_ID_SAS_01, stBswComData.canMsgBuffer[RX_MSG_IDX_SAS_01]);

    Rte_Call_BswCom_rEcuAbsCan_setPduInfo(TX_MSG_IDX_SDC_01, TX_MSG_LEN_SDC_01, TX_MSG_ID_SDC_01, stBswComData.canMsgBuffer[TX_MSG_IDX_SDC_01]);
    Rte_Call_BswCom_rEcuAbsCan_setPduInfo(TX_MSG_IDX_PHY_RES, TX_MSG_LEN_PHY_RES, TX_MSG_ID_PHY_RES, stBswComData.canMsgBuffer[TX_MSG_IDX_PHY_RES]);
    Rte_Call_BswCom_rEcuAbsCan_setPduInfo(TX_MSG_IDX_CCP_RES, TX_MSG_LEN_CCP_RES, TX_MSG_ID_CCP_RES, stBswComData.canMsgBuffer[TX_MSG_IDX_CCP_RES]);
    Rte_Call_BswCom_rEcuAbsCan_setPduInfo(TX_MSG_IDX_CCP_MON0, TX_MSG_LEN_CCP_MON_DEFAULT, TX_MSG_ID_CCP_MON0, stBswComData.canMsgBuffer[TX_MSG_IDX_CCP_MON0]);
    Rte_Call_BswCom_rEcuAbsCan_setPduInfo(TX_MSG_IDX_CCP_MON1, TX_MSG_LEN_CCP_MON_DEFAULT, TX_MSG_ID_CCP_MON1, stBswComData.canMsgBuffer[TX_MSG_IDX_CCP_MON1]);
    Rte_Call_BswCom_rEcuAbsCan_setPduInfo(TX_MSG_IDX_CCP_MON2, TX_MSG_LEN_CCP_MON_DEFAULT, TX_MSG_ID_CCP_MON2, stBswComData.canMsgBuffer[TX_MSG_IDX_CCP_MON2]);
    Rte_Call_BswCom_rEcuAbsCan_setPduInfo(TX_MSG_IDX_CCP_MON3, TX_MSG_LEN_CCP_MON_DEFAULT, TX_MSG_ID_CCP_MON3, stBswComData.canMsgBuffer[TX_MSG_IDX_CCP_MON3]);
    Rte_Call_BswCom_rEcuAbsCan_setPduInfo(TX_MSG_IDX_CCP_MON4, TX_MSG_LEN_CCP_MON_DEFAULT, TX_MSG_ID_CCP_MON4, stBswComData.canMsgBuffer[TX_MSG_IDX_CCP_MON4]);
    Rte_Call_BswCom_rEcuAbsCan_setPduInfo(TX_MSG_IDX_CCP_MON5, TX_MSG_LEN_CCP_MON_DEFAULT, TX_MSG_ID_CCP_MON5, stBswComData.canMsgBuffer[TX_MSG_IDX_CCP_MON5]);
    Rte_Call_BswCom_rEcuAbsCan_setPduInfo(TX_MSG_IDX_CCP_MON6, TX_MSG_LEN_CCP_MON_DEFAULT, TX_MSG_ID_CCP_MON6, stBswComData.canMsgBuffer[TX_MSG_IDX_CCP_MON6]);
    Rte_Call_BswCom_rEcuAbsCan_setPduInfo(TX_MSG_IDX_CCP_MON7, TX_MSG_LEN_CCP_MON_DEFAULT, TX_MSG_ID_CCP_MON7, stBswComData.canMsgBuffer[TX_MSG_IDX_CCP_MON7]);
}

//========================================
// 🔄 [메인 루프] 계속 반복 실행되는 함수
// 1️⃣ 데이터 받기 → 2️⃣ 데이터 정리 → 3️⃣ 데이터 보내기 → 4️⃣ 에러 체크
//========================================
void REtBswCom_bswComMain(void)
{
	processingDataRx();		// ↓ 1️⃣ 받은 센서 데이터를 의미있는 값으로 추출
	processingDataTx();		// ↓ 2️⃣ 응답 메시지 준비
	sendCanTx();			// ↓ 3️⃣ 응답 메시지 전송 (100ms마다)
	if(stSas01.sas01TestValue01 == 0xFF)
	{
	    while(1);			// ↓ 4️⃣ 비정상 값(0xFF) 감지하면 멈춤 (에러)
	}
}

//========================================
// 🚨 [이벤트 함수] CAN 메시지 수신시 자동으로 호출됨
// "센서에서 메시지 왔어!" → 이 함수가 즉시 실행
//========================================
void REcbBswCom_canRxIndication(void)
{
	uint8 handle;			// 📌 어떤 메시지인지 구분하는 ID
	uint8 length;			// 📌 메시지의 길이 (몇 바이트?)
	uint32 id;				// 📌 CAN 메시지 ID
	uint8 data[MAX_CAN_MSG_DLC];  // 📌 실제 메시지 데이터

	UT_getCanRxIndicationData(&handle, &length, &id, data);  // "뭐가 왔어?"

	if(handle == RX_MSG_IDX_SAS_01)  // ↓ 스티어링 센서 메시지만 받으면
	{
		(void)memcpy(stSas01.message, data, length);  // → stSas01에 저장
	}
    else if(handle == RX_MSG_IDX_FUN_REQ)  // ↓ 진단 요청 메시지면
    {
        Rte_Call_BswCom_rDcmCmd_processDiagRequest(FALSE, data, length);  // → 진단 모듈에 전달
    }
    else if(handle == RX_MSG_IDX_PHY_REQ)  // ↓ 물리적 진단 요청이면
    {
        Rte_Call_BswCom_rDcmCmd_processDiagRequest(TRUE, data, length);  // → 진단 모듈에 전달
    }
    else if(handle == RX_MSG_IDX_CCP_REQ)  // ↓ CCP 캘리브레이션 요청이면
    {
        Rte_Call_BswCom_rCcpCustomCmd_processCcpRequest(data);  // → CCP 모듈에 전달
    }
    else
    {
        (void) 0;  // → 다른 메시지는 무시
    }
}

//========================================
// 📊 [데이터 추출] 바이너리 → 의미있는 값으로 변환
// 예: 01 2F → 스티어링 45도, 온도 23도
//========================================
static void processingDataRx(void)
{
	// stSas01.message는 바이너리 데이터 (01 2F A5 같은 형태)
	// 아래 3줄은 이 바이너리에서 필요한 값만 추출하는 것
	
	UT_readSignalData(stSas01.message, 0u, 8u, TRUE, DataType_uint8, &stSas01.sas01TestValue01);
	// ↑ 0번 비트부터 8비트 추출 → sas01TestValue01에 저장 (예: 45도)
	
	UT_readSignalData(stSas01.message, 22u, 12u, TRUE, DataType_uint16, &stSas01.sas01TestValue02);
	// ↑ 22번 비트부터 12비트 추출 → sas01TestValue02에 저장 (예: 온도)
	
	UT_readSignalData(stSas01.message, 62u, 12u, FALSE, DataType_uint16, &stSas01.sas01TestValue03);
	// ↑ 62번 비트부터 12비트 추출 → sas01TestValue03에 저장 (예: 상태)
}

//========================================
// 🔄 [응답 메시지 준비] 센서값을 복사해서 응답 메시지 만들기
// 센서: "스티어링 45도로 변했어"
// 우리: "알겠습니다, 45도 받았습니다" 메시지 만들기
//========================================
static void processingDataTx(void)
{
	// 📋 Step 1: 받은 센서값을 우리의 응답 메시지에 복사 (에코)
	stSdc01.echoSas01TestValue02 = stSas01.sas01TestValue02;  // 45도 복사
	stSdc01.echoSas01TestValue03 = stSas01.sas01TestValue03;  // 온도 복사
	
	// 📋 Step 2: 응답 메시지 초기화 (전체를 0으로 채우기)
	(void)memset(stSdc01.message, 0, TX_MSG_LEN_SDC_01);  // 깨끗하게 비우기
	
	// 📋 Step 3: 준비한 값들을 바이너리 형식으로 변환
	UT_writeSignalData(stSdc01.message, 0u, 8u, TRUE, DataType_uint8, &stSdc01.aliveCount);
	// ↑ aliveCount를 0번 비트부터 8비트로 변환
	
	UT_writeSignalData(stSdc01.message, 22u, 12u, TRUE, DataType_uint16, &stSdc01.echoSas01TestValue02);
	// ↑ 복사한 센서값을 22번 비트부터 12비트로 변환
	
	UT_writeSignalData(stSdc01.message, 62u, 12u, FALSE, DataType_uint16, &stSdc01.echoSas01TestValue03);
	// ↑ 복사한 센서값을 62번 비트부터 12비트로 변환
}

//========================================
// 📤 [전송] 응답 메시지를 100ms마다 CAN으로 전송
// 카운터: 0,1,2,...,99,100(전송!) → 0,1,...,99,100(전송!)
//========================================
static void sendCanTx(void)
{
	static uint32 countCanTx = 0u;  // 📊 카운터 (0부터 시작, 계속 증가)
	
	countCanTx++;  // ⬆️ 1씩 증가 (0→1→2→...→99→100→1→...→99→100)
	
	if((countCanTx % TIME_DECI_100MS) == 0)  // ✅ 100ms 주기마다 (countCanTx가 100의 배수가 되면)
	{
		Rte_Call_BswCom_rEcuAbsCan_canSend(TX_MSG_IDX_SDC_01, stSdc01.message);
		// ↑ 준비된 응답 메시지(stSdc01.message)를 CAN으로 전송
		
		stSdc01.aliveCount++;  // ✅ "살아있어요" 신호 증가 (1→2→3→4...)
		// 이것은 센서에 "나 살아있고 정상 작동 중이야"라고 알려주는 신호
	}
}
