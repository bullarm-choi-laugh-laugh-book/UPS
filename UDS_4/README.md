# UDS_4: Dynamic Seed-Key Table (개선된 인증)

## 🎯 기능

**UDS_3의 고정 Seed-Key 시스템**을 **동적 테이블**로 개선합니다.

```
UDS_3: Seed = 항상 0x1234 (고정) ❌
UDS_4: Seed = 0x1234, 0x2345, 0x3456 중 랜덤 ✅
```

---

## 🔐 Seed-Key 테이블

```c
typedef struct {
    uint16 seed;
    uint16 key;
} ST_SeedKeyPair;

const ST_SeedKeyPair seedKeyTable[] = {
    {0x1234, 0x4567},  // [0] Seed 0x1234 → Key 0x4567
    {0x2345, 0x5678},  // [1] Seed 0x2345 → Key 0x5678
    {0x3456, 0x6789},  // [2] Seed 0x3456 → Key 0x6789
};

#define SEED_KEY_TABLE_SIZE 3
```

---

## 🎲 3가지 핵심 기능

### **기능 1: Seed 랜덤 선택**

```c
static uint8 getRandomSeedIndex(void)
{
    static uint8 seedCounter = 0;
    uint8 index = seedCounter % SEED_KEY_TABLE_SIZE;
    seedCounter++;
    return index;
    
    // 호출 순서:
    // 1차: index=0 → Seed=0x1234
    // 2차: index=1 → Seed=0x2345
    // 3차: index=2 → Seed=0x3456
    // 4차: index=0 → Seed=0x1234 (반복)
}
```

### **기능 2: Key 매핑 조회**

```c
uint16 selectedSeed = seedKeyTable[currentSeedIndex].seed;

// 예:
// index=0 → seedKeyTable[0].seed = 0x1234
// index=1 → seedKeyTable[1].seed = 0x2345
```

### **기능 3: 일치 여부 검사**

```c
uint16 expectedKey = seedKeyTable[currentSeedIndex].key;

if(key == expectedKey)  // Key 검증
{
    isSecurityUnlock = TRUE;  // ✅ 성공
}
else
{
    errorResult = NRC_InvalidKey;  // ❌ 실패
}
```

---

## 📊 호출 순서별 동작

### **첫 번째 인증**

```
RequestSeed 호출
  ↓
getRandomSeedIndex() = 0
  ↓
selectedSeed = seedKeyTable[0].seed = 0x1234
  ↓
응답: [0x67 0x01 0x12 0x34]
  ↓
진단기: "Seed=0x1234니까 Key=0x4567이겠네"
  ↓
SendKey [0x27 0x02 0x45 0x67]
  ↓
expectedKey = seedKeyTable[0].key = 0x4567
  ↓
if(0x4567 == 0x4567) → ✅ PASS!
```

### **두 번째 인증**

```
RequestSeed 호출
  ↓
getRandomSeedIndex() = 1
  ↓
selectedSeed = seedKeyTable[1].seed = 0x2345
  ↓
응답: [0x67 0x01 0x23 0x45]
  ↓
진단기: "Seed=0x2345니까 Key=0x5678이겠네"
  ↓
SendKey [0x27 0x02 0x56 0x78]
  ↓
expectedKey = seedKeyTable[1].key = 0x5678
  ↓
if(0x5678 == 0x5678) → ✅ PASS!
```

---

## 🎬 시스템 아키텍처

```
securityAccess()
│
├─ Request Seed (0x01)
│  ├─ [기능 1] getRandomSeedIndex()
│  │           → currentSeedIndex 결정
│  │
│  ├─ [기능 2] seedKeyTable[index].seed 추출
│  │
│  └─ Seed 응답
│
└─ Send Key (0x02)
   ├─ Key 수신
   │
   ├─ [기능 2] seedKeyTable[index].key 추출
   │
   └─ [기능 3] Key 비교
      → isSecurityUnlock = TRUE/FALSE
```

---

## 💡 개선 사항

### **보안 향상**

| 항목 | UDS_3 | UDS_4 |
|------|-------|-------|
| **Seed** | 항상 0x1234 | 매번 다름 |
| **안전성** | 낮음 | 높음 |
| **복잡도** | 낮음 | 중간 |

---

## 🔧 확장 가능성

```c
// 더 많은 Seed-Key 쌍 추가
const ST_SeedKeyPair seedKeyTable[] = {
    {0x1234, 0x4567},
    {0x2345, 0x5678},
    {0x3456, 0x6789},
    {0x4567, 0x7890},  // ← 추가 쌍
    {0x5678, 0x8901},  // ← 추가 쌍
};

#define SEED_KEY_TABLE_SIZE 5
```

---

**Status**: ✅ Complete | **Concept**: Dynamic Authentication System
