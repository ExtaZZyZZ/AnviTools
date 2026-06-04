# AnviTools

Десктопное приложение на Qt5 (C++) для диагностики и управления встроенными CAN-устройствами серии Anvi. Обеспечивает связь по шине CAN через адаптер Waveshare USB-CAN-A и мониторинг отладочного UART-вывода прошивки.

---

## Назначение

Приложение предназначено для работы с тремя модулями:

| Модуль | Node ID | Назначение |
|--------|---------|------------|
| Autonomous Module | 0x02 | Автономный модуль управления |
| AnviDrive | 0x07 | Контроллер привода/клапана |
| AnviEnergy | 0x0A | Система управления аккумулятором (BMS) |

---

## Требования к сборке

- **Qt** 5.x (`core`, `gui`, `widgets`, `serialport`)
- **Компилятор:** MSVC 2022 (Windows)
- **Менеджер пакетов:** vcpkg (переменная окружения `VCPKG_ROOT`)
- **Стандарт C++:** C++14
- **Платформа:** Windows (только)

> Debug-сборка принудительно переключается в Release из-за ограничений vcpkg.

---

## Сборка

```bash
# Открыть ModemDebugger.pro в Qt Creator и собрать,
# либо через командную строку:
qmake ModemDebugger.pro
nmake           # или mingw32-make для MinGW
```

Исполняемый файл: `AnviTools.exe`  
Готовая сборка с зависимостями: папка `AnviTools с .exe файлом/`

---

## Архитектура

```
AnviTools/
├── can/            # Аппаратный драйвер CAN-адаптера Waveshare USB-CAN-A
├── canopen/        # CANopen SDO-клиент (чтение/запись OD)
├── serial/         # Монитор отладочного UART2 (115 200 бод)
├── model/          # Структуры данных (FlashLogEntry, DeviceConfig)
├── ui/             # Все Qt-виджеты и вкладки
├── resources/      # Иконки и ресурсы (.qrc)
├── main.cpp        # Точка входа
└── ModemDebugger.pro  # Qt-проект
```

### Ключевые классы

| Класс | Файл | Описание |
|-------|------|---------|
| `CanDriver` | `can/CanDriver.*` | Бинарный протокол Waveshare: фреймы `0xAA ... 0x55`, 20 байт, скорость 2 000 000 бод |
| `SdoClient` | `canopen/SdoClient.*` | CANopen SDO (expedited + segmented), блокирующий через QEventLoop |
| `SerialMonitor` | `serial/SerialMonitor.*` | Чтение UART2 в отдельном потоке, сигнал `lineReceived()` по QueuedConnection |
| `MainWindow` | `ui/MainWindow.*` | Главное окно, три вкладки, адаптивная вёрстка |
| `ModuleHubTab` | `ui/ModuleHubTab.*` | Сканирование модулей по NMT-heartbeat, маршрутизация |
| `FirmwareTab` | `ui/FirmwareTab.*` | Загрузка прошивки через кастомный бутлоадер, CRC32, стейт-машина |

### Поток данных

```
UI Tabs
  │
  ├── CanDriver (основной поток) ──▶ SdoClient (блокирующие SDO-вызовы)
  │       │                                │
  │   frameReceived                   CAN-фреймы
  │                                        │
  └── SerialMonitor (worker-поток) ──▶ MonitorTab (QueuedConnection)
```

---

## Протоколы

### CAN-адаптер (CanDriver)
- Формат фрейма: `0xAA [тип] [IDL] [IDH] [данные 0–8] 0x55`
- Фиксированная длина 20 байт с контрольной суммой
- Поддержка 11-бит (Standard) и 29-бит (Extended) CAN ID
- COM-порт CH341, скорость 2 000 000 бод

### CANopen SDO (SdoClient)
- Request COB-ID: `0x600 + nodeId`
- Response COB-ID: `0x580 + nodeId`
- Expedited (1–4 байта) и сегментированный (>4 байт) режимы
- Тайм-аут по умолчанию: 1000 мс

### Бутлоадер прошивки (FirmwareTab)
| Команда | COB-ID TX | Байт | Описание |
|---------|-----------|------|----------|
| CMD_ENTER | 0x7E0 | 0xB0 | Запрос входа в бутлоадер |
| CMD_START | 0x7E2 | 0x01 | Адрес + размер прошивки |
| CMD_DATA  | 0x7E2 | 0x02 | 6 байт данных на фрейм |
| CMD_END   | 0x7E2 | 0x03 | CRC32 |
| CMD_JUMP  | 0x7E2 | 0x04 | Запуск приложения |

- RSP COB-ID: `0x7E3`
- Базовый адрес STM32: `0x08008000`
- Тайм-ауты: 3000 мс (boot/done), 500 мс (ack)

---

## Вкладки интерфейса

| Вкладка | Назначение |
|---------|-----------|
| **Подключение** | Выбор COM-портов CAN и UART2, диагностика сырых байт |
| **Монитор** | Цветной вывод отладочного UART (`[OK]` зелёный, `[ERR]` красный, `[WARN]` жёлтый и т.д.) |
| **Модули** | Сводные карточки модулей, NMT-статус, CAN-монитор фреймов |
| **AnviDrive** | Управление приводом: кнопки Открыть/Стоп/Закрыть, телеметрия PDO, Valve Travel |
| **AnviEnergy** | BMS: SoC, температуры, напряжения, токи, конфигурация |
| **Autonomous** | Режим сна, SPI, стирание флэш, управление пингом, конфиг APN, белый список |
| **SDO-консоль** | Универсальный ручной SDO read/write по Node ID + Index + Subindex |
| **Flash Log** | Считывание и экспорт в CSV лог-записей из флэш-памяти устройства |

---

## Object Dictionary (OD) — основные индексы

### Autonomous Module (Node 0x02)
| Index | Описание |
|-------|----------|
| 0x6000 | Allow Sleep |
| 0x6003 | Whitelist (телефоны) |
| 0x6005 | SPI Switch |
| 0x6006 | APN (primary) |
| 0x6007 | Poll Interval |
| 0x6009 | Erase Flash |
| 0x600A | APN2 (backup) |
| 0x600B | Connectivity Check Interval |
| 0x600C | Ping Mode |
| 0x6008 | Save to Flash |
| 0x2020 | Flash Log — Trigger dump |
| 0x2021 | Flash Log — Entry (8 байт) |
| 0x2022 | Flash Log — Remaining count |

### AnviDrive (Node 0x07) — PDO
| COB-ID | OD | Данные |
|--------|-----|--------|
| 0x487 | 0x6001 | Percent (REAL32) |
| 0x287 | 0x6002 | Motor speed (INT16) |
| 0x687 | 0x6003 | Electrodrive speed (INT16) |
| 0x187 | 0x6004–0x6007 | Temps/humidity (4×INT16) |

### AnviEnergy (Node 0x0A) — PDO
| COB-ID | OD | Данные |
|--------|-----|--------|
| 0x28A | 0x600C–0x600E | tempElec, humElec, tempAkku (3×INT16) |
| 0x18A | 0x600F–0x6012 | SoC (INT32) |

---

## Заметки для разработчика

- Все вызовы `SdoClient` **блокирующие** (внутренний QEventLoop). На время SDO-операции UI-кнопки должны быть заблокированы во избежание re-entrancy.
- `SerialMonitor` обязательно перемещается в отдельный `QThread` **до** вызова `open()`.
- CAN-драйвер использует `QMutex` для потокобезопасной отправки.
- Название проекта в `.pro` файле — `ModemDebugger` (историческое), исполняемый файл — `AnviTools`.
- Стиль UI: тема Fusion, цвета `#9964BB` / `#440486` (фиолетовые), шрифт Bahnschrift 10pt.

---

## Файл проекта

`ModemDebugger.pro` — основной файл сборки Qt. Содержит пути включения, список исходников и зависимость от vcpkg.

`ModemDebugger.pro.user` — пользовательские настройки Qt Creator (IDE-specific, не коммитить при смене машины).
