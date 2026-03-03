# UDS_2: ECU Reset (0x11 서비스)

## 🎯 기능

**ECU Reset**은 마이크로컨트롤러를 재부팅하는 서비스입니다.

```
진단기: "넌 재부팅 해줄래?"
   ↓
ECU: "알겠습니다, 재부팅!"
   ↓
🔄 시스템 전체 초기화
```

---

## 🔄 동작 흐름

### **Step 1: BswDcm에서 Reset 플래그 세팅**

```c
static uint8 ecuReset(void)
{
    if(subFunctionId == SFID_EcuReset_HardReset)  // 0x01
    {
        stBswDcmData.stDiagStatus.isEcuReset = TRUE;  // ← 플래그 세팅!
    }
    // 응답: [0x51 0x01] 성공
}
```

### **Step 2: EcuAbs에서 실제 Reset 실행**

```c
void REtEcuAbs_ecuAbsMain(void)
{
    ST_DiagStatus stDiagStatus;
    Rte_Call_BswCom_rDcmCmd_getDiagStatus(&stDiagStatus);
    
    if(stDiagStatus.isEcuReset == TRUE)  // ← 플래그 확인
    {
        Mcu_PerformReset();  // ← 실제 Reset 실행!
    }
}
```

---

## 📊 시퀀스 다이어그램

```
진단기          BswCom         BswDcm         EcuAbs
  │              │              │              │
  ├─[0x11 0x01]─→│              │              │
  │              ├─ 전달        │              │
  │              │              │              │
  │              │            ecuReset()       │
  │              │            isEcuReset=TRUE │
  │              │              │              │
  │              │              ├─ getDiagStatus() 호출
  │              │              │              │
  │              │              │              ├─ Mcu_PerformReset()
  │              │              │              │
  │              │              │              🔄 RESET!
  │←─────────────[0x51 0x01]───┤              │
  │              (응답)         │              │
```

---

## 📊 블랙박스 테스트 결과

### **UDS_1 vs UDS_2**

| 항목 | UDS_1 | UDS_2 |
|------|-------|-------|
| **Reset 명령** | 미지원 ❌ | 지원 ✅ |
| **응답** | [0x7F...] 에러 | [0x51] 성공 |
| **실제 동작** | 안 됨 | 🔄 시스템 재부팅 |

---

## 💡 의의

- **진단 시스템의 완성도 향상**
- **auto reset 기능** → 유지보수 효율성 ⬆️
- EcuAbs와의 **계층 간 통신** 구현

---

## 🔃 BswDcm과 EcuAbs의 상호작용

```
BswDcm의 역할:
- 진단 명령 처리
- 상태 플래그(isEcuReset) 세팅
- 응답 메시지 생성

EcuAbs의 역할:
- BswDcm의 상태 조회 (getDiagStatus)
- 실제 하드웨어 제어
- Reset 실행
```

---

**Status**: ✅ Complete | **Files**: Rte_BswDcm.c, Rte_BswCom.c, Rte_EcuAbs.c
