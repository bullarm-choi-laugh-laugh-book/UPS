# UDS_1: Session Control (0x10 서비스)

## 🎯 기능

**Session Control**은 진단 세션을 전환하는 UDS 기본 서비스입니다.

```
Default Session (기본)
     ↕ (0x10)
Extended Session (확장)
```

---

## 📋 핵심 코드

### **Rte_BswDcm.c**

```c
// 세션 설정
enum {
    DIAG_SESSION_DEFAULT,    // 0 - 기본 모드
    DIAG_SESSION_EXTENDED    // 1 - 확장 모드
};

// 세션 변경 처리
static uint8 diagnosticSessionControl(void)
{
    uint8 subFunctionId = stBswDcmData.diagDataBufferRx[1];
    
    if(subFunctionId == 0x01)  // Default로 변경
        stBswDcmData.stDiagStatus.currentSession = DIAG_SESSION_DEFAULT;
    else if(subFunctionId == 0x03)  // Extended로 변경
        stBswDcmData.stDiagStatus.currentSession = DIAG_SESSION_EXTENDED;
}
```

### **Rte_BswCom.c**

```c
// CAN 통신 기본 처리
void REcbBswCom_canRxIndication(void)
{
    // 진단 요청을 BswDcm에 전달
    Rte_Call_BswCom_rDcmCmd_processDiagRequest(TRUE, data, length);
}
```

---

## 🔄 동작 흐름

```
진단기: [0x10 0x03] (Extended 요청)
   ↓
BswCom: CAN 수신
   ↓
BswDcm: diagnosticSessionControl() 호출
   ↓
currentSession = DIAG_SESSION_EXTENDED ✅
   ↓
응답: [0x50 0x03]
```

---

## 📊 출력 (블랙박스 테스트)

```
[진단기] → [0x10 0x03]
[ECU]    ← [0x50 0x03]

Function List:
- Start Diag (Default)    ✅ Pass
- Start Diag (Extended)   ✅ Pass
```

---

## 💡 의의

- **가장 기본이 되는 UDS 서비스**
- Extended 세션으로 진전해야 고급 기능 사용 가능
- 보안: Default에서는 제한된 기능만 가능

---

**Status**: ✅ Complete | **Files**: Rte_BswDcm.c, Rte_BswCom.c
