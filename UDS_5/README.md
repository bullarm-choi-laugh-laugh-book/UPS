# UDS_5: Communication Control (0x28 서비스)

## 🎯 기능

**진단 중에 CAN 메시지 전송을 제어**합니다.

```
UDS_1: 진단 세션 변경
UDS_2: ECU 리셋
UDS_3: 보안 잠금해제
UDS_4: 동적 Seed-Key
UDS_5: CAN 전송 ON/OFF ← 새로운 기능!
```

---

## 🎛️ Communication Control 서비스

### **서비스 ID: 0x28**

| 기능 | ServiceID | SubFunction | 효과 |
|------|-----------|------------|------|
| **enableRxTx** | 0x28 | 0x00 | RX/TX 모두 활성화 |
| **enableRxDisableTx** | 0x28 | 0x01 | **RX만 활성화** (센서 수신O, 응답O) |
| **disableRxEnableTx** | 0x28 | 0x02 | TX만 활성화 (센서 수신X, 응답O) |
| **disableRxTx** | 0x28 | 0x03 | **RX/TX 모두 비활성화** (모든 전송 중지) |

```
진단기 명령:
  [0x28 0x03] → "센서 메시지 보내지 마!"
  ↓
ECU 응답:
  [0x68 0x03]
  ↓
효과: 센서값 '얼려짐'
  ↓
진단기 명령:
  [0x28 0x01] → "다시 센서 보내!"
  ↓
ECU 응답:
  [0x68 0x01]
  ↓
효과: 센서값 '흐른다'
```

---

## 🔄 코드 구조

### **UDS_3, UDS_4까지의 구조**

```c
// Rte_BswCom.c - sendCanTx() 함수
static void sendCanTx(void)
{
    // CAN 메시지를 항상 전송
    REtEcuAbs_sendCanMessage(0x001, message);
}
```

### **UDS_5의 개선된 구조** ✨

```c
// Rte_BswCom.c - sendCanTx() 함수 (개선)
static void sendCanTx(void)
{
    // BswDcm에서 현재 진단 상태 조회
    ST_DiagStatus stDiagStatus;
    REoiBswDcm_getDiagStatus(&stDiagStatus);
    
    // ← ← [핵심] 통신 제어 플래그 확인
    if(stDiagStatus.isCommunicationDisable == FALSE)
    {
        // 통신 활성화 상태 → CAN 메시지 전송
        REtEcuAbs_sendCanMessage(0x001, message);
    }
    // 해석: isCommunicationDisable=FALSE → 전송O
    //      isCommunicationDisable=TRUE  → 전송X
}
```

---

## 🎬 시나리오: 센서 값 '얼리기'

### **Step 1: 정상 상태 (센서값 흐른다)**

```
Time: 0ms
sendCanTx() 호출
  ↓
isCommunicationDisable = FALSE
  ↓
if(FALSE == FALSE) → TRUE ✅
  ↓
REtEcuAbs_sendCanMessage() 실행
  ↓
CAN 메시지 전송됨 ✨
  ↓
진단기 모니터: 
  Speed [5km/h] → [7km/h] → [9km/h] (계속 변함)
```

### **Step 2: [0x28 0x03] 명령 실행**

```
Time: 100ms
진단기: [0x28 0x03] 명령 전송
  ↓
BswDcm.communicationControlHandler()
  ↓
if(SubFunction == 0x03)  // disableRxTx
{
    isCommunicationDisable = TRUE;  // ← ← 플래그 활성화
}
  ↓
ECU 응답: [0x68 0x03]
```

### **Step 3: 센서 값 '얼려짐'**

```
Time: 200ms
sendCanTx() 호출
  ↓
isCommunicationDisable = TRUE
  ↓
if(TRUE == FALSE) → FALSE ❌
  ↓
REtEcuAbs_sendCanMessage() 스킵됨!
  ↓
CAN 메시지 전송 안됨 🛑
  ↓
진단기 모니터:
  Speed [9km/h] → [9km/h] → [9km/h] (멈춤!)
```

### **Step 4: [0x28 0x01] 명령으로 재개**

```
Time: 3000ms (3초 후)
진단기: [0x28 0x01] 명령 전송
  ↓
BswDcm.communicationControlHandler()
  ↓
if(SubFunction == 0x01)  // enableRxDisableTx
{
    isCommunicationDisable = FALSE;  // ← ← 플래그 비활성화
}
  ↓
ECU 응답: [0x68 0x01]
  ↓
이후 sendCanTx(): if(FALSE == FALSE) → TRUE ✅
  ↓
CAN 메시지 다시 전송됨 🟢
  ↓
진단기 모니터:
  Speed [9km/h] → [11km/h] → [13km/h] (다시 흐른다!)
```

---

## 📊 타이밍 다이어그램

```
Time:     0ms        100ms       200ms       3000ms      3100ms
          ↓          ↓           ↓           ↓           ↓
명령      -          [0x28 0x03] -           [0x28 0x01] -
          
응답      -          [0x68 0x03] -           [0x68 0x01] -

센서      [변함] → [변함] → [얼림] → [얼림] → [얼림] → [변함] → [변함]
          5km/h     7km/h    9km/h    9km/h    9km/h   11km/h  13km/h

플래그    FALSE     TRUE           TRUE       FALSE      FALSE
(Disable)           (시작)                    (끝)

전송상태  ✅ 전송  ✅ 응답  🛑 중지   🛑 중지   🛑 중지   ✅ 재개  ✅ 전송
```

---

## 💼 실제 사용 사례

### **1️⃣ 진단 분석**
```
진단기: "센서값을 얼려서 정확하게 측정하고 싶어"
→ [0x28 0x03] 전송
→ 센서값 고정됨
→ 정확한 분석 가능 ✅
```

### **2️⃣ 액추에이터 테스트**
```
진단기: "RX한테만 제어신호 보낼거야 (응답은 안 받아도 되고)"
→ [0x28 0x02] (enableRxDisableTx)
→ 센서 수신O, 응답X
→ ECU 내부에만 명령령이 적용됨
```

### **3️⃣ 통신 복구**
```
진단기: "다시 정상 통신 해!"
→ [0x28 0x00] or [0x28 0x01]
→ CAN 메시지 흐름 재개 ✅
```

---

## 🔗 계층 간 통신

```
BswCom (통신 계층)
│
├─ sendCanTx()
│  ├─ [호출]
│  │
│  └─ REoiBswDcm_getDiagStatus(&stDiagStatus)
│     ↓
│     BswDcm (진단 계층)
│     ├─ getDiagStatus()
│     │  └─ stDiagStatus.isCommunicationDisable (플래그)
│     │
│     └─ [반환값]: isCommunicationDisable = TRUE/FALSE
│        ↑
│        communicationControlHandler()에서 설정됨
│
└─ [조건] if(isCommunicationDisable == FALSE)
   → REtEcuAbs_sendCanMessage() 호출
```

---

## 🎓 학습 진행도

| 버전 | 기능 | 목적 | 복잡도 |
|------|------|------|--------|
| **UDS_1** | Session Control | 진단 모드 변경 | ⭐ |
| **UDS_2** | ECU Reset | 하드웨어 리셋 | ⭐ |
| **UDS_3** | Security Access | 고정 인증 | ⭐⭐ |
| **UDS_4** | Dynamic Seed-Key | 동적 인증 | ⭐⭐ |
| **UDS_5** | Comm Control | 메시지 제어 | ⭐⭐ |

---

**Status**: ✅ Complete | **Concept**: Transmission Control System
