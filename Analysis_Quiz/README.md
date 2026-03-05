# UDS (Unified Diagnostic Services) 학습 프로젝트

## 📚 개요

이 프로젝트는 **자동차 OTA(Over-The-Air) 부트로더 및 UDS 진단 서비스**를 학습하는 교육 자료입니다.

UDS_1부터 UDS_5까지 단계별로 구현하며, 각 단계마다 새로운 진단 기능을 추가합니다.

---

## 🎯 학습 목표

- ✅ UDS 기본 개념 이해
- ✅ Session Control 구현
- ✅ ECU Reset 기능
- ✅ Security Access (인증) 시스템
- ✅ Seed-Key 기반 동적 인증
- ✅ Communication Control (통신 제어)

---

## 📁 폴더 구조

```
UDS_1/  → Session Control (기본)
UDS_2/  → Session Control + ECU Reset
UDS_3/  → + Security Access (고정 Seed-Key)
UDS_4/  → + Security Access (동적 Seed-Key 테이블)
UDS_5/  → + Communication Control (통신 제어)
```

---

## 🔄 각 단계별 설명

### **UDS_1: Session Control (0x10 서비스)**

**기능:**
- Default Session ↔ Extended Session 전환

**코드:**
- `Rte_BswDcm.c` - 세션 제어 로직
- `Rte_BswCom.c` - CAN 통신 관리

**동작:**
```
진단기: [0x10 0x03] (Extended 세션 요청)
   ↓
ECU: currentSession = EXTENDED 세팅
   ↓
응답: [0x50 0x03] (성공)
```

---

### **UDS_2: ECU Reset (0x11 서비스)**

**기능:**
- Session Control + ECU Reset
- System Reset 명령 처리

**추가 코드:**
- `ecuReset()` 함수
- `Rte_EcuAbs.c` - 실제 Reset 실행

**동작:**
```
진단기: [0x11 0x01] (Reset 요청)
   ↓
ECU: isEcuReset = TRUE 세팅
   ↓
EcuAbs: Mcu_PerformReset() 호출
   ↓
🔄 시스템 재부팅
```

---

### **UDS_3: Security Access (0x27 서비스)**

**기능:**
- 2단계 인증 (Seed-Key)
- Extended 세션에서만 활성화

**동작:**

**Step 1: Request Seed (0x01)**
```
진단기: [0x27 0x01] 
   ↓
ECU: seed = 0x1234 반환
   ↓
응답: [0x67 0x01 0x12 0x34]
```

**Step 2: Send Key (0x02)**
```
진단기: [0x27 0x02 0x56 0x78] (Key 전송)
   ↓
ECU: if(key == 0x5678) ✅ 성공
   ↓
응답: [0x67 0x02]
isSecurityUnlock = TRUE
```

---

### **UDS_4: Seed-Key Table (동적 인증)**

**기능:**
- 여러 Seed-Key 쌍 관리
- 매번 다른 Seed 발송

**Seed-Key 테이블:**
```
┌─────────────┬─────────────┐
│    Seed     │     Key     │
├─────────────┼─────────────┤
│   0x1234    │   0x4567    │
│   0x2345    │   0x5678    │
│   0x3456    │   0x6789    │
└─────────────┴─────────────┘
```

**3가지 기능:**
1. **Seed 랜덤 선택** - `getRandomSeedIndex()`
2. **Key 매핑 조회** - `seedKeyTable[index].key`
3. **일치 여부 검사** - `if(key == expectedKey)`

---

### **UDS_5: Communication Control (0x28 서비스)**

**기능:**
- ECU의 CAN 통신 ON/OFF 제어
- 진단 분석 시 센서값 고정

**동작:**

**통신 중지 (0x28 0x03):**
```
진단기: [0x28 0x03]
   ↓
ECU: isCommunicationDisable = TRUE
   ↓
sendCanTx()에서 송신 중단
   ↓
센서값 고정 (데이터 분석 용이)
```

**통신 재개 (0x28 0x01):**
```
진단기: [0x28 0x01]
   ↓
ECU: isCommunicationDisable = FALSE
   ↓
다시 메시지 송신 시작
```

---

## 🏗️ 시스템 아키텍처

```
┌─────────────────────────────────────┐
│      BswCom (통신 계층)              │
│  - CAN 송수신                       │
│  - 센서 데이터 처리                  │
│  - 진단 요청 전달                    │
└────────────┬────────────────────────┘
             │
             ▼
┌─────────────────────────────────────┐
│      BswDcm (진단 계층)              │
│  - Session Control (0x10)           │
│  - ECU Reset (0x11)                 │
│  - Security Access (0x27)           │
│  - Communication Control (0x28)     │
└────────────┬────────────────────────┘
             │
             ▼
┌─────────────────────────────────────┐
│      EcuAbs (하드웨어 계층)          │
│  - 마이크로컨트롤러 제어             │
│  - Reset 실행                       │
│  - CAN 송수신                       │
└─────────────────────────────────────┘
```

---

## 📊 블랙박스 테스트 비교

### **UDS_1 vs UDS_2: Reset 기능**

| 항목 | UDS_1 | UDS_2 |
|------|-------|-------|
| **Reset 요청** | [0x11] | [0x11] |
| **응답** | ❌ [0x7F 0x11] (미지원) | ✅ [0x51] (성공) |
| **실제 동작** | 안 됨 | 시스템 재부팅 |

### **UDS_3 vs UDS_5: 인증 및 통신 제어**

| 기능 | UDS_3 | UDS_5 |
|------|-------|-------|
| **Security Access** | ✅ | ✅ |
| **Communication Control** | ❌ | ✅ |
| **센서값 고정** | 불가능 | 가능 |
| **진단 효율** | 중간 | 높음 |

---

## 🔐 보안 아키텍처

```
┌─────────────────────────────────────┐
│   Default Session                   │
│  (기본 모드 - 제한적 기능)            │
└─────────────────────────────────────┘
              ↓ (0x10 0x03)
┌─────────────────────────────────────┐
│   Extended Session                  │
│  (확장 모드 - 고급 기능 가능)         │
│                                     │
│  ├─ Security Access 가능            │
│  │  ├─ Request Seed (0x27 0x01)    │
│  │  └─ Send Key (0x27 0x02)        │
│  │                                  │
│  └─ 인증 성공                       │
│     └─ isSecurityUnlock = TRUE      │
│        (메모리 읽기/쓰기 가능)       │
└─────────────────────────────────────┘
```

---

## 🎬 실제 진단 시퀀스 예시

```
[진단기]                    [ECU]
  │                          │
  ├─ [0x10 0x03] ─────────→ (Extended 세션)
  │                          │
  ├─ [0x27 0x01] ─────────→ (RequestSeed)
  │                    ┌───────────┐
  │                    │ Seed 선택 │
  │                    │ = 0x1234  │
  │                    └───────────┘
  │                          │
  │ ← [0x67 0x01 0x12 0x34]─┤
  │ (Seed 받음)              │
  │                          │
  │ (계산: Key 결정)          │
  │  0x1234 → 0x4567         │
  │                          │
  ├─ [0x27 0x02 0x45 0x67]─→ (SendKey)
  │                    ┌───────────┐
  │                    │ Key 검증  │
  │                    │ 일치! ✅  │
  │                    │ Unlock!   │
  │                    └───────────┘
  │                          │
  │ ← [0x67 0x02] ──────────┤
  │ (인증 성공)              │
  │                          │
  ├─ [0x28 0x03] ─────────→ (통신 중지)
  │ (데이터 분석)            │
  ├─ [0x22 ID] ────────────→ (읽기 요청)
  │ ← [0x62 ID DATA] ────────┤
  │                          │
  ├─ [0x28 0x01] ─────────→ (통신 재개)
  │                          │
```

---

## 💡 주요 개념

### **Session (세션)**
- **Default**: 정상 운영 모드 (제한된 기능)
- **Extended**: 개발/테스트/캘리브레이션 모드 (확장 기능)

### **Seed-Key 인증**
- **Seed**: ECU가 진단기에 주는 값
- **Key**: 진단기가 ECU에 보내는 값
- **검증**: Seed-Key 쌍이 일치해야만 인증 성공

### **Communication Control**
- **ON**: 메시지 계속 전송 (정상 운영)
- **OFF**: 메시지 송신 중단 (분석/테스트 모드)

---

## 🚀 활용 분야

1. **OTA (Over-The-Air) 업데이트**
   - 펌웨어 다운로드 전 확인

2. **모듈 진단 및 테스트**
   - 각 시스템 상태 확인
   - 결함 진단

3. **보안**
   - 무단 접근 방지
   - 인증된 사용자만 기능 사용

4. **개발 및 디버깅**
   - 센서값 고정으로 정확한 분석
   - 원격 ECU 제어

---

## 📝 파일 설명

### **BswDcm (Basic Software Diagnostic Communication Manager)**
- 진단 요청 처리
- 각 서비스 구현 (Session, Reset, Security, Communication)

### **BswCom (Basic Software Communication)**
- CAN 메시지 송수신
- 센서 데이터 처리
- 진단 요청 전달

### **EcuAbs (ECU Abstraction)**
- 하드웨어 제어
- Reset 실행
- 타이머 관리

---

## 🎓 학습 진행도

- [x] UDS 기본 개념
- [x] Session Control 구현
- [x] ECU Reset 구현
- [x] Security Access 구현
- [x] Seed-Key Table 설계
- [x] Communication Control 구현
- [x] 블랙박스 테스트
- [ ] 실제 하드웨어 테스트
- [ ] OTA 통합

---

## 🔗 참고 자료

- **UDS 표준**: ISO 14229-1
- **CAN 통신**: ISO 11898
- **AUTOSAR**: AUTomotive Open System ARchitecture

---

## ✨ 특징

**진단 계층 분리**
- 기능별로 명확하게 구분
- 확장과 유지보수 용이

**단계별 학습**
- UDS_1부터 UDS_5까지 점진적 학습
- 각 단계마다 새로운 개념 추가

**실무 기반**
- 실제 자동차 진단 시스템과 유사
- 프로토타입 개발에 활용 가능

---

## 📞 문의

각 코드의 상세 설명은 코드 내 주석 참고

---

**작성일**: 2026년 3월 3일  
**학습자**: Bootloader & OTA 연구팀
