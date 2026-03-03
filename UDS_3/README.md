# UDS_3: Security Access (0x27 서비스)

## 🎯 기능

**Security Access**는 **2단계 인증 시스템**입니다.

```
Step 1: Request Seed (0x01)
진단기 ← ECU가 특정 값(Seed) 제공

Step 2: Send Key (0x02)  
진단기 → ECU에 계산한 값(Key) 전송

Seed-Key 일치 → 인증 성공! ✅
```

---

## 🔐 Seed-Key 테이블

| Seed | Key | 의미 |
|------|-----|------|
| 0x1234 | 0x5678 | 고정 매핑 (테스트용) |

---

## 🔄 인증 절차

### **Step 1: Request Seed (0x27 0x01)**

```c
static uint8 securityAccess(void)
{
    if(subFunctionId == SFID_SecurityAccess_RequestSeed)  // 0x01
    {
        uint16 seed = 0x1234;  // ← Seed 결정
        
        // 응답 메시지에 Seed 포함
        stBswDcmData.diagDataBufferTx[3] = (uint8)((seed & 0xFF00u) >> 8u);  // High Byte
        stBswDcmData.diagDataBufferTx[4] = (uint8)(seed & 0xFFu);              // Low Byte
        
        // 응답: [0x67 0x01 0x12 0x34]
    }
}
```

### **Step 2: Send Key (0x27 0x02)**

```c
else if(subFunctionId == SFID_SecurityAccess_SendKey)  // 0x02
{
    uint16 key = ((uint16)stBswDcmData.diagDataBufferRx[2] << 8u) | 
                  stBswDcmData.diagDataBufferRx[3];
    
    // Key 검증
    if(key == 0x5678u)  // ← 정확한 Key!
    {
        stBswDcmData.stDiagStatus.isSecurityUnlock = TRUE;  // ✅ 인증 성공!
    }
    else
    {
        errorResult = NRC_InvalidKey;  // ❌ 인증 실패
    }
}
```

---

## 📊 시퀀스

```
진단기              ECU
  │               │
  ├─[0x27 0x01]──→ (RequestSeed)
  │               │
  │            seed=0x1234
  │               │
  │←─[0x67 0x01]──┤
  │    0x12 0x34  │
  │ (Seed 받음)   │
  │               │
  │ (계산해서...  │
  │  Key=0x5678) │
  │               │
  ├─[0x27 0x02]──→ (SendKey)
  │    0x56 0x78  │
  │               │
  │            key==0x5678?
  │            YES! ✅
  │            isSecurityUnlock=TRUE
  │               │
  │←─[0x67 0x02]──┤
  │ (성공!)        │
```

---

## 🚨 조건

**Extended 세션에서만 동작:**

```c
if(stBswDcmData.stDiagStatus.currentSession == DIAG_SESSION_DEFAULT)
{
    // Default 세션에서는 차단!
    errorResult = NRC_ServiceNotSupportedInActiveSession;
}
```

---

## 💡 의의

- **무단 접근 방지** 🔒
- **인증된 사용자만** 중요 기능 사용 가능
- 2단계 인증으로 **보안 강화**

---

**Status**: ✅ Complete | **Files**: Rte_BswDcm_SecurityAccess.c, Rte_BswCom.c
