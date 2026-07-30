/*
  ESP32 Dual Bluetooth Module Laboratory - Split A/B Scan v8.4b
  ======================================

  שינויים מרכזיים ב-v8.4b
  -----------------
  - נוספה אזהרת מתח ברורה לתלמידים בראש הקוד ובממשק המשתמש.
  - HC-05 / HC-06 מוזנים מ-5V בהתאם ללוחות המעבדה.
  - HM-10 Clone / HMSoft BT4.0 מוזנים מ-3.3V מה-ESP32.
  - נוסף קצב 230400 baud לסריקות UART.
  - ממפה הבאוד של HMSoft משתמש בזמני המתנה מותאמים לקצב,
    בניסיונות חוזרים ובסריקת Recovery לפני שחזור הקצב המקורי.
  - אישורי Y/N אינם תלויים עוד באות גדולה או קטנה:
    Y/y מאשרים ו-N/n מבטלים.
  - קלט אחר אינו מבטל פעולה בטעות, אלא גורם לבקשה חוזרת.
  - מיפוי קודי הבאוד של HMSoft נשאר זמין בתפריט HMSoft באמצעות T.
  - ממפה HMSoft בטוח: קוד 7 חסום ואינו נשלח למודול; נבדקים 0-6 ואז 8.
  - קוד 7 מסומן כמסוכן בעקבות בדיקת החומרה ואינו נכלל במיפוי האוטומטי.
  - בתפריט HC-05: N = שינוי שם המודול.
  - בתפריט הראשי: V = מסוף ידני של Module B.
  - בכל כניסה לתפריט HC-05 מוצג prompt מפורש של ההקשר.

  מטרת התוכנית
  -------------
  חיבור שני מודולי UART-Bluetooth לאותו ESP32 וטיפול סימטרי בכל אחד מהם.

  כל אחד משני הערוצים, A ו-B, מסוגל:
  - לסרוק קצבי UART נפוצים.
  - לזהות את סיום השורה הנדרש: None / CR / LF / CRLF.
  - לזהות HC-05 במצב Full AT.
  - לזהות HC-06 עם קושחת linvor.
  - לזהות HM-10 Clone / MLT-BT05-like עם קושחות כגון v6.1/v6.3.
  - לפתוח תפריט תצורה מלא המתאים לסוג המודול שזוהה.
  - לפתוח מסוף AT ידני.

  בנוסף, כאשר שני המודולים הם HM-10 Clone:
  - ניתן להגדיר A כ-Master ו-B כ-Slave.
  - ניתן לבצע INQ ממושך.
  - ניתן להאזין בו-זמנית לשני ערוצי ה-UART.

  ==========================================================
  הגדרות Serial Monitor
  ==========================================================
  Baud rate: 115200
  Line ending: כל אפשרות מתקבלת
  מומלץ: Both NL & CR

  חשוב:
  הגדרת Line Ending במוניטור משפיעה רק על התקשורת
  בין המחשב ל-ESP32.

  התוכנית מזהה בנפרד מה כל מודול דורש:
  - HC-05 Full AT: בדרך כלל 38400 ו-CRLF
  - HC-06 linvor: בדרך כלל None
  - HM-10 Clone v6.x: בדרך כלל CRLF

  ==========================================================
  חיבורים
  ==========================================================

  Module A:
  Module A TX  -> ESP32 GPIO16  (RX1)
  Module A RX  <- ESP32 GPIO17  (TX1)
  Module A GND -> ESP32 GND

  Module B:
  Module B TX  -> ESP32 GPIO26  (RX2)
  Module B RX  <- ESP32 GPIO27  (TX2)
  Module B GND -> ESP32 GND

  חובה לחבר GND משותף לכל המערכת.

  שימו לב:
  TX של המודול מתחבר ל-RX של ה-ESP32.
  RX של המודול מתחבר ל-TX של ה-ESP32.

  ==========================================================
  אזהרת מתח חשובה לתלמידים
  ==========================================================
  קווי UART של ESP32 הם 3.3V.

  במערך המעבדה הזה:
  - HC-05 / HC-06: הזנת VCC של 5V.
  - HM-10 Clone / HMSoft BT4.0: הזנת VCC של 3.3V
    מהדק 3V3 של ה-ESP32.

  לפני החלפת סוג המודול חובה לנתק מתח ולבדוק מחדש
  את חיבור VCC. אין לחבר HM-10 Clone או HMSoft BT4.0
  להזנת 5V במערך המעבדה הזה.

  חובה לחבר GND משותף למודול ול-ESP32.

  ==========================================================
  מיפוי קודי BAUD
  ==========================================================
  +BAUD=1  // 1200 baud
  +BAUD=2  // 2400 baud
  +BAUD=3  // 4800 baud
  +BAUD=4  // 9600 baud
  +BAUD=5  // 19200 baud
  +BAUD=6  // 38400 baud
  +BAUD=7  // 57600 baud
  +BAUD=8  // 115200 baud

  פקודות שינוי:
  AT+BAUD1  // 1200 baud
  AT+BAUD2  // 2400 baud
  AT+BAUD3  // 4800 baud
  AT+BAUD4  // 9600 baud
  AT+BAUD5  // 19200 baud
  AT+BAUD6  // 38400 baud
  AT+BAUD7  // 57600 baud
  AT+BAUD8  // 115200 baud
*/

#include <Arduino.h>

// ==========================================================
// הגדרת פינים ושני ממשקי UART
// ==========================================================

constexpr int MODULE_A_RX_PIN = 16;
constexpr int MODULE_A_TX_PIN = 17;

constexpr int MODULE_B_RX_PIN = 26;
constexpr int MODULE_B_TX_PIN = 27;

HardwareSerial ModuleASerial(1);
HardwareSerial ModuleBSerial(2);

// ==========================================================
// קבועי זמן
// ==========================================================

constexpr unsigned long NORMAL_TIMEOUT_MS = 850;

// Some HC-05 firmware versions answer AT+NAME? unusually slowly.
// This longer timeout is used only for HC-05 name read-back attempts.
constexpr unsigned long HC05_NAME_TIMEOUT_MS = 2200;
constexpr unsigned long HC05_NAME_QUIET_TIME_MS = 220;
constexpr unsigned long HELP_TIMEOUT_MS = 5000;
constexpr unsigned long QUIET_TIME_MS = 130;
constexpr unsigned long HELP_QUIET_TIME_MS = 350;
constexpr unsigned long USB_IDLE_MS = 80;
constexpr unsigned long INQUIRY_LISTEN_MS = 30000;

// ==========================================================
// קצבי UART הנסרקים
// ==========================================================

const uint32_t BAUD_RATES[] =
{
  1200,
  2400,
  4800,
  9600,
  19200,
  38400,
  57600,
  115200,
  230400
};

constexpr size_t BAUD_COUNT =
  sizeof(BAUD_RATES) / sizeof(BAUD_RATES[0]);

// ==========================================================
// טיפוסי מידע
// ==========================================================

enum class LineEnding
{
  NONE,
  CR,
  LF,
  CRLF
};

enum class ModuleType
{
  UNKNOWN,
  HC05_FULL_AT,
  HC06_LINVOR,
  HM10_CLONE,
  HMSOFT_V006
};

struct BluetoothModule
{
  char label;
  HardwareSerial* uart;
  int rxPin;
  int txPin;

  uint32_t baud;
  LineEnding ending;
  ModuleType type;

  String version;
  String help;
  bool detected;
};

BluetoothModule moduleA =
{
  'A',
  &ModuleASerial,
  MODULE_A_RX_PIN,
  MODULE_A_TX_PIN,
  0,
  LineEnding::NONE,
  ModuleType::UNKNOWN,
  "",
  "",
  false
};

BluetoothModule moduleB =
{
  'B',
  &ModuleBSerial,
  MODULE_B_RX_PIN,
  MODULE_B_TX_PIN,
  0,
  LineEnding::NONE,
  ModuleType::UNKNOWN,
  "",
  "",
  false
};

// Forward declarations for the extended low-baud probe used by the main scan.
unsigned long hmSoftProbeTimeoutMs(uint32_t baud);
unsigned long hmSoftQuietTimeMs(uint32_t baud);


// ==========================================================
// פונקציות עזר כלליות
// ==========================================================

const char* endingName(LineEnding ending)
{
  switch (ending)
  {
    case LineEnding::NONE: return "None";
    case LineEnding::CR:   return "CR";
    case LineEnding::LF:   return "LF";
    case LineEnding::CRLF: return "CRLF";
  }

  return "Unknown";
}

String endingText(LineEnding ending)
{
  switch (ending)
  {
    case LineEnding::NONE: return "";
    case LineEnding::CR:   return "\r";
    case LineEnding::LF:   return "\n";
    case LineEnding::CRLF: return "\r\n";
  }

  return "";
}

const char* moduleTypeName(ModuleType type)
{
  switch (type)
  {
    case ModuleType::HC05_FULL_AT:
      return "HC-05 in full AT mode";

    case ModuleType::HC06_LINVOR:
      return "HC-06 with linvor firmware";

    case ModuleType::HM10_CLONE:
      return "HM-10 compatible BLE clone";

    case ModuleType::HMSOFT_V006:
      return "HM-10 / HMSoft firmware";

    default:
      return "Unknown module";
  }
}

String makeReadable(const String& data)
{
  String result;

  for (size_t i = 0; i < data.length(); i++)
  {
    uint8_t value = static_cast<uint8_t>(data[i]);

    if (value == '\r')
    {
      result += "<CR>";
    }
    else if (value == '\n')
    {
      result += "<LF>";
    }
    else if (value == '\t')
    {
      result += "<TAB>";
    }
    else if (value >= 32 && value <= 126)
    {
      result += static_cast<char>(value);
    }
    else
    {
      char buffer[8];
      snprintf(buffer, sizeof(buffer), "<%02X>", value);
      result += buffer;
    }
  }

  return result;
}

void printHex(const String& data)
{
  Serial.print("    HEX: ");

  for (size_t i = 0; i < data.length(); i++)
  {
    uint8_t value = static_cast<uint8_t>(data[i]);

    if (value < 0x10)
    {
      Serial.print('0');
    }

    Serial.print(value, HEX);
    Serial.print(' ');
  }

  Serial.println();
}

void clearModuleInput(BluetoothModule& module)
{
  while (module.uart->available())
  {
    module.uart->read();
  }
}

void openModuleUart(
  BluetoothModule& module,
  uint32_t baud,
  uint32_t serialConfig = SERIAL_8N1
)
{
  module.uart->end();
  delay(60);

  module.uart->begin(
    baud,
    serialConfig,
    module.rxPin,
    module.txPin
  );

  delay(180);
  clearModuleInput(module);
}

String readModuleResponse(
  BluetoothModule& module,
  unsigned long timeoutMs,
  unsigned long quietTimeMs = QUIET_TIME_MS
)
{
  String response;
  unsigned long startTime = millis();
  unsigned long lastByteTime = startTime;
  bool receivedAnyByte = false;

  while (millis() - startTime < timeoutMs)
  {
    while (module.uart->available())
    {
      response += static_cast<char>(module.uart->read());
      lastByteTime = millis();
      receivedAnyByte = true;
    }

    if (receivedAnyByte &&
        millis() - lastByteTime >= quietTimeMs)
    {
      break;
    }

    delay(1);
  }

  return response;
}

String sendCommand(
  BluetoothModule& module,
  const String& command,
  LineEnding ending,
  unsigned long timeoutMs = NORMAL_TIMEOUT_MS,
  unsigned long quietTimeMs = QUIET_TIME_MS
)
{
  clearModuleInput(module);

  module.uart->print(command);
  module.uart->print(endingText(ending));
  module.uart->flush();

  return readModuleResponse(
    module,
    timeoutMs,
    quietTimeMs
  );
}

String sendDetectedCommand(
  BluetoothModule& module,
  const String& command,
  unsigned long timeoutMs = NORMAL_TIMEOUT_MS,
  unsigned long quietTimeMs = QUIET_TIME_MS
)
{
  return sendCommand(
    module,
    command,
    module.ending,
    timeoutMs,
    quietTimeMs
  );
}

String executeAndDisplay(
  BluetoothModule& module,
  const String& command,
  unsigned long timeoutMs = NORMAL_TIMEOUT_MS,
  unsigned long quietTimeMs = QUIET_TIME_MS
)
{
  Serial.println();
  Serial.print("Module ");
  Serial.print(module.label);
  Serial.print(" command: ");
  Serial.println(command);

  String response = sendDetectedCommand(
    module,
    command,
    timeoutMs,
    quietTimeMs
  );

  Serial.print("Response: ");

  if (response.length() == 0)
  {
    Serial.println("<no response>");
  }
  else
  {
    Serial.println(makeReadable(response));
  }

  return response;
}

bool isReadableText(const String& text)
{
  if (text.length() == 0)
  {
    return false;
  }

  for (size_t i = 0; i < text.length(); i++)
  {
    uint8_t value = static_cast<uint8_t>(text[i]);

    if (!((value >= 32 && value <= 126) ||
          value == '\r' ||
          value == '\n' ||
          value == '\t'))
    {
      return false;
    }
  }

  return true;
}

// ==========================================================
// קליטת פקודות מה-Serial Monitor
// ==========================================================

String readUsbCommand()
{
  static String buffer;
  static unsigned long lastInputTime = 0;

  while (Serial.available())
  {
    char incomingByte =
      static_cast<char>(Serial.read());

    lastInputTime = millis();

    if (incomingByte == '\r' ||
        incomingByte == '\n')
    {
      if (buffer.length() > 0)
      {
        String command = buffer;
        buffer = "";
        command.trim();
        return command;
      }
    }
    else
    {
      buffer += incomingByte;
    }
  }

  if (buffer.length() > 0 &&
      millis() - lastInputTime >= USB_IDLE_MS)
  {
    String command = buffer;
    buffer = "";
    command.trim();
    return command;
  }

  return "";
}

String waitForInput()
{
  while (true)
  {
    String command = readUsbCommand();

    if (command.length() > 0)
    {
      return command;
    }

    delay(1);
  }
}

bool confirmAction()
{
  while (true)
  {
    Serial.println("Enter Y/y to confirm or N/n to cancel:");

    String answer = waitForInput();
    answer.trim();

    if (answer.equalsIgnoreCase("Y"))
    {
      return true;
    }

    if (answer.equalsIgnoreCase("N"))
    {
      return false;
    }

    Serial.println(
      "Invalid input. Please enter Y/y or N/n."
    );
  }
}

// ==========================================================
// טיפול בקודי BAUD
// ==========================================================

uint32_t baudFromCode(char code)
{
  switch (code)
  {
    case '1': return 1200;
    case '2': return 2400;
    case '3': return 4800;
    case '4': return 9600;
    case '5': return 19200;
    case '6': return 38400;
    case '7': return 57600;
    case '8': return 115200;
    default:  return 0;
  }
}

const char* baudDescription(char code)
{
  switch (code)
  {
    case '1': return "1200 baud";
    case '2': return "2400 baud";
    case '3': return "4800 baud";
    case '4': return "9600 baud";
    case '5': return "19200 baud";
    case '6': return "38400 baud";
    case '7': return "57600 baud";
    case '8': return "115200 baud";
    default:  return "unknown baud";
  }
}

void explainBaudResponse(const String& response)
{
  int marker = response.indexOf("+BAUD=");

  if (marker < 0 ||
      marker + 6 >= static_cast<int>(response.length()))
  {
    return;
  }

  char code = response[marker + 6];

  Serial.print("Interpreted value: +BAUD=");
  Serial.print(code);
  Serial.print(" means ");
  Serial.println(baudDescription(code));
}

void showBaudMenu()
{
  Serial.println("1 - 1200 baud");
  Serial.println("2 - 2400 baud");
  Serial.println("3 - 4800 baud");
  Serial.println("4 - 9600 baud");
  Serial.println("5 - 19200 baud");
  Serial.println("6 - 38400 baud");
  Serial.println("7 - 57600 baud");
  Serial.println("8 - 115200 baud");
  Serial.println("X - Cancel");
}

// ==========================================================
// סריקה וזיהוי
// ==========================================================

int scoreResponse(const String& response)
{
  if (response.length() == 0)
  {
    return 0;
  }

  String upper = response;
  upper.toUpperCase();

  int score = 0;

  if (upper.indexOf("OK") >= 0)
  {
    score += 5;
  }

  if (upper.indexOf("ERROR") >= 0)
  {
    score += 2;
  }

  if (upper.indexOf("LINVOR") >= 0)
  {
    score += 5;
  }

  if (upper.indexOf("+VERSION") >= 0)
  {
    score += 5;
  }

  if (isReadableText(response))
  {
    score += 1;
  }

  return score;
}

bool scanOneModule(BluetoothModule& module)
{
  module.detected = false;
  module.baud = 0;
  module.type = ModuleType::UNKNOWN;
  module.version = "";
  module.help = "";

  Serial.println();
  Serial.println("======================================");
  Serial.print(" Scanning Module ");
  Serial.println(module.label);
  Serial.println("======================================");

  /*
    ----------------------------------------------------------
    Stage 1: dedicated HC-05 full-AT pre-check
    ----------------------------------------------------------

    A standard HC-05 in full AT mode normally uses:
      38400 baud
      CRLF line ending

    We test this exact combination before the universal scan.

    This prevents two opposite problems:
    1. Sending "AT" without an ending can leave a partial command
       in the HC-05 parser.
    2. Sending a clearing CRLF before every universal test can
       disturb HC-06 modules, which expect "AT" with no ending.
  */

  openModuleUart(module, 38400);

  /*
    Do not send a preliminary empty CRLF line here.

    On the tested HC-05 firmware, an empty CRLF can itself disturb the
    command parser. The successful sequence is a clean AT command followed
    directly by CRLF.
  */
  clearModuleInput(module);
  delay(120);

  String hc05Response = sendCommand(
    module,
    "AT",
    LineEnding::CRLF,
    900,
    150
  );

  /*
    Retry once after reopening UART. This helps when the ESP32 UART was
    restarted while the HC-05 remained powered in full AT mode.
  */
  if (hc05Response.length() == 0)
  {
    openModuleUart(module, 38400);
    delay(250);

    hc05Response = sendCommand(
      module,
      "AT",
      LineEnding::CRLF,
      900,
      150
    );
  }

  Serial.println();
  Serial.println("Dedicated HC-05 full-AT check:");
  Serial.print("  38400 / CRLF: ");

  if (hc05Response.length() == 0)
  {
    Serial.println("<no response>");
  }
  else
  {
    Serial.println(makeReadable(hc05Response));
    printHex(hc05Response);
  }

  String hc05Upper = hc05Response;
  hc05Upper.toUpperCase();

  if (hc05Upper.indexOf("OK") >= 0)
  {
    module.baud = 38400;
    module.ending = LineEnding::CRLF;
    module.detected = true;

    Serial.println();
    Serial.println("*** POSITIVE HC-05 AT RESPONSE FOUND ***");
    Serial.print("Module: ");
    Serial.println(module.label);
    Serial.println("Baud: 38400");
    Serial.println("Ending: CRLF");
    Serial.print("Response: ");
    Serial.println(makeReadable(hc05Response));

    return true;
  }

  /*
    ----------------------------------------------------------
    Stage 2: universal scan
    ----------------------------------------------------------

    The original order is restored:
      None, CR, LF, CRLF

    This is essential for HC-06 linvor, which normally requires
    AT without any line-ending characters.
  */

  const LineEnding endings[] =
  {
    // NONE is tested first by the extended probe below.
    LineEnding::CR,
    LineEnding::LF,
    LineEnding::CRLF
  };

  for (size_t baudIndex = 0;
       baudIndex < BAUD_COUNT;
       baudIndex++)
  {
    uint32_t currentBaud =
      BAUD_RATES[baudIndex];

    openModuleUart(module, currentBaud);

    Serial.println();
    Serial.print("Testing baud: ");
    Serial.println(currentBaud);

    /*
      Extended no-line-ending probe.

      HMSoft V006 and HC-06 commonly answer AT with no line ending.
      Low UART rates require visibly longer receive windows, so the main
      scan now performs three explicit attempts with a baud-dependent
      timeout before trying CR, LF and CRLF.
    */
    const uint8_t extendedAttempts = 3;

    for (uint8_t attempt = 1; attempt <= extendedAttempts; attempt++)
    {
      unsigned long timeoutMs = hmSoftProbeTimeoutMs(currentBaud);
      unsigned long quietMs = hmSoftQuietTimeMs(currentBaud);

      Serial.print("  Extended NONE probe ");
      Serial.print(attempt);
      Serial.print("/");
      Serial.print(extendedAttempts);
      Serial.print(" - timeout ");
      Serial.print(timeoutMs);
      Serial.println(" ms");

      clearModuleInput(module);

      String response = sendCommand(
        module,
        "AT",
        LineEnding::NONE,
        timeoutMs,
        quietMs
      );

      int currentScore = scoreResponse(response);

      Serial.print("    Response: ");
      if (response.length() == 0)
      {
        Serial.println("<no response>");
      }
      else
      {
        Serial.println(makeReadable(response));
        printHex(response);
      }

      Serial.print("    Score: ");
      Serial.println(currentScore);

      String currentUpper = response;
      currentUpper.toUpperCase();

      if (currentUpper.indexOf("OK") >= 0)
      {
        module.baud = currentBaud;
        module.ending = LineEnding::NONE;
        module.detected = true;

        Serial.println();
        Serial.println("*** POSITIVE AT RESPONSE FOUND ***");
        Serial.print("Module: ");
        Serial.println(module.label);
        Serial.print("Baud: ");
        Serial.println(module.baud);
        Serial.println("Ending: None");
        Serial.print("Response: ");
        Serial.println(makeReadable(response));

        return true;
      }

      delay(currentBaud <= 2400 ? 250 : 100);
    }

    int bestScore = 0;
    LineEnding bestEnding = LineEnding::NONE;
    String bestResponse;

    for (LineEnding ending : endings)
    {
      /*
        Do not send a preliminary CRLF here.

        HC-06 expects a clean "AT" with no ending. A preliminary
        CRLF can make some HC-06 boards ignore the following test.
      */
      clearModuleInput(module);

      String response = sendCommand(
        module,
        "AT",
        ending,
        750
      );

      int currentScore = scoreResponse(response);

      Serial.print("  ");
      Serial.print(endingName(ending));
      Serial.print(": ");

      if (response.length() == 0)
      {
        Serial.println("<no response>");
      }
      else
      {
        Serial.println(makeReadable(response));
        printHex(response);
      }

      Serial.print("    Score: ");
      Serial.println(currentScore);

      if (currentScore > bestScore)
      {
        bestScore = currentScore;
        bestEnding = ending;
        bestResponse = response;
      }

      /*
        Stop immediately after a valid OK response.

        This prevents a successfully detected HC-05 or HC-06 from being
        disturbed by additional tests using the wrong line ending.
      */
      String currentUpper = response;
      currentUpper.toUpperCase();

      if (currentUpper.indexOf("OK") >= 0)
      {
        module.baud = currentBaud;
        module.ending = ending;
        module.detected = true;

        Serial.println();
        Serial.println("*** POSITIVE AT RESPONSE FOUND ***");
        Serial.print("Module: ");
        Serial.println(module.label);
        Serial.print("Baud: ");
        Serial.println(module.baud);
        Serial.print("Ending: ");
        Serial.println(endingName(module.ending));
        Serial.print("Response: ");
        Serial.println(makeReadable(response));

        return true;
      }

      /*
        Give modules time to finish processing one attempt before
        the next line-ending style is tested.
      */
      delay(180);
      clearModuleInput(module);
    }

    Serial.println();

    if (bestScore > 0)
    {
      Serial.print("Best result at ");
      Serial.print(currentBaud);
      Serial.print(" baud: Ending = ");
      Serial.print(endingName(bestEnding));
      Serial.print(", Score = ");
      Serial.println(bestScore);

      Serial.print("Response = ");
      Serial.println(makeReadable(bestResponse));
    }
    else
    {
      Serial.print("Best result at ");
      Serial.print(currentBaud);
      Serial.println(
        " baud: No valid readable response."
      );
    }


  }

  Serial.println();
  Serial.print("Module ");
  Serial.print(module.label);
  Serial.println(" was not detected.");

  return false;
}

String probe(
  BluetoothModule& module,
  const String& command,
  unsigned long timeoutMs = NORMAL_TIMEOUT_MS,
  unsigned long quietTimeMs = QUIET_TIME_MS
)
{
  Serial.println();
  Serial.print("--- Module ");
  Serial.print(module.label);
  Serial.print(" probe: ");
  Serial.print(command);
  Serial.println(" ---");

  String response = sendDetectedCommand(
    module,
    command,
    timeoutMs,
    quietTimeMs
  );

  Serial.print("Response: ");

  if (response.length() == 0)
  {
    Serial.println("<no response>");
  }
  else
  {
    Serial.println(makeReadable(response));
  }

  return response;
}

void identifyOneModule(BluetoothModule& module)
{
  if (!module.detected)
  {
    return;
  }

  Serial.println();
  Serial.println("======================================");
  Serial.print(" Identifying Module ");
  Serial.println(module.label);
  Serial.println("======================================");

  /*
    Different Bluetooth-module families use different firmware commands:

    HC-06 linvor:
      AT+VERSION

    HM-10 clone / MLT-BT05-like:
      AT+VERSION

    HMSoft firmware:
      AT+VERS?

    HC-05 in full AT mode:
      AT+VERSION?
  */

  module.version = probe(module, "AT+VERSION");

  String versionUpper = module.version;
  versionUpper.toUpperCase();

  if (versionUpper.indexOf("LINVOR") >= 0)
  {
    module.type = ModuleType::HC06_LINVOR;
  }
  else if (versionUpper.indexOf("+VERSION=V") >= 0)
  {
    module.help = probe(
      module,
      "AT+HELP",
      HELP_TIMEOUT_MS,
      HELP_QUIET_TIME_MS
    );

    String typeResponse = probe(module, "AT+TYPE");
    String uuidResponse = probe(module, "AT+UUID");
    String charResponse = probe(module, "AT+CHAR");

    String typeUpper = typeResponse;
    String uuidUpper = uuidResponse;
    String charUpper = charResponse;

    typeUpper.toUpperCase();
    uuidUpper.toUpperCase();
    charUpper.toUpperCase();

    bool cloneSignature =
      typeUpper.indexOf("+TYPE=") >= 0 &&
      uuidUpper.indexOf("+UUID=") >= 0 &&
      charUpper.indexOf("+CHAR=") >= 0;

    if (cloneSignature)
    {
      module.type = ModuleType::HM10_CLONE;
    }
  }

  /*
    HMSoft firmware uses AT+VERS? rather than AT+VERSION.
    Example verified in the laboratory:
      AT+VERS? -> HMSoft V006
  */
  if (module.type == ModuleType::UNKNOWN)
  {
    String hmSoftVersion = probe(module, "AT+VERS?");
    String hmSoftUpper = hmSoftVersion;
    hmSoftUpper.toUpperCase();

    if (hmSoftUpper.indexOf("HMSOFT") >= 0)
    {
      module.type = ModuleType::HMSOFT_V006;
      module.version = hmSoftVersion;

      // Non-destructive confirmation probes.
      probe(module, "AT+NAME?");
      probe(module, "AT+BAUD?");
      probe(module, "AT+ROLE?");
      probe(module, "AT+ADDR?");
    }
  }

  /*
    A standard HC-05 in full AT mode normally works at:
    38400 baud, CRLF, and responds to AT+VERSION?.
  */
  if (module.type == ModuleType::UNKNOWN)
  {
    String hc05Version = probe(module, "AT+VERSION?");
    String hc05Role = probe(module, "AT+ROLE?");

    String hc05VersionUpper = hc05Version;
    String hc05RoleUpper = hc05Role;

    hc05VersionUpper.toUpperCase();
    hc05RoleUpper.toUpperCase();

    bool hc05Signature =
      hc05VersionUpper.indexOf("+VERSION:") >= 0 ||
      hc05RoleUpper.indexOf("+ROLE:") >= 0;

    if (hc05Signature)
    {
      module.type = ModuleType::HC05_FULL_AT;
      module.version = hc05Version;
    }
  }

  Serial.println();
  Serial.println("======================================");
  Serial.print(" Module ");
  Serial.print(module.label);
  Serial.println(" detection result");
  Serial.println("======================================");
  Serial.print("Detected baud: ");
  Serial.println(module.baud);
  Serial.print("Detected module line ending: ");
  Serial.println(endingName(module.ending));
  Serial.print("Firmware response: ");
  Serial.println(makeReadable(module.version));
  Serial.print("Estimated module type: ");
  Serial.println(moduleTypeName(module.type));

  if (module.type == ModuleType::HC05_FULL_AT)
  {
    Serial.println(
      "HC-05 note: keep KEY/button active during power-up for full AT mode."
    );
  }
  else if (module.type == ModuleType::HMSOFT_V006)
  {
    Serial.println(
      "HMSoft note: this firmware uses query commands such as AT+VERS? and AT+ADDR?."
    );
  }

  Serial.println("======================================");
}

void scanAndIdentifyOne(BluetoothModule& module)
{
  /*
    Scan and identify only the selected UART channel.

    Important:
    This function changes only the selected module record.
    The stored detection data of the other module is preserved.
  */
  scanOneModule(module);
  identifyOneModule(module);
}

void scanAndIdentifyBoth()
{
  scanAndIdentifyOne(moduleA);
  scanAndIdentifyOne(moduleB);
}

// ==========================================================
// הצגת מצב
// ==========================================================

void showModuleStatus(const BluetoothModule& module)
{
  Serial.println();
  Serial.println("--------------------------------------");
  Serial.print("Module ");
  Serial.println(module.label);
  Serial.println("--------------------------------------");

  Serial.print("Detected: ");
  Serial.println(module.detected ? "Yes" : "No");

  if (!module.detected)
  {
    return;
  }

  Serial.print("Type: ");
  Serial.println(moduleTypeName(module.type));

  Serial.print("Baud: ");
  Serial.println(module.baud);

  Serial.print("Module line ending: ");
  Serial.println(endingName(module.ending));

  Serial.print("Version: ");
  Serial.println(makeReadable(module.version));
}

void showBothStatus()
{
  showModuleStatus(moduleA);
  showModuleStatus(moduleB);
}

// ==========================================================
// מנגנון שינוי ואימות
// ==========================================================

bool successfulResponse(const String& response)
{
  if (response.length() == 0)
  {
    return false;
  }

  String upper = response;
  upper.toUpperCase();

  return
    upper.indexOf("OK") >= 0 ||
    upper.indexOf("+NAME=") >= 0 ||
    upper.indexOf("+PIN=") >= 0 ||
    upper.indexOf("+TYPE=") >= 0 ||
    upper.indexOf("+ROLE=") >= 0 ||
    upper.indexOf("+POWE=") >= 0 ||
    upper.indexOf("+UUID=") >= 0 ||
    upper.indexOf("+CHAR=") >= 0 ||
    upper.indexOf("+BAUD=") >= 0;
}

void sendConfirmed(
  BluetoothModule& module,
  const String& command,
  const String& readBackCommand
)
{
  Serial.println();
  Serial.print("Command to send to Module ");
  Serial.print(module.label);
  Serial.print(": ");
  Serial.println(command);

  if (!confirmAction())
  {
    Serial.println("Change cancelled.");
    return;
  }

  String response =
    executeAndDisplay(module, command);

  if (!successfulResponse(response))
  {
    Serial.println(
      "The module did not clearly confirm the change."
    );
  }

  delay(1000);

  if (readBackCommand.length() == 0)
  {
    return;
  }

  String readBack =
    executeAndDisplay(module, readBackCommand);

  if (readBack.length() == 0)
  {
    Serial.println(
      "No read-back response. Retrying after 1 second..."
    );

    delay(1000);

    readBack =
      executeAndDisplay(module, readBackCommand);
  }

  if (readBack.indexOf("+BAUD=") >= 0)
  {
    explainBaudResponse(readBack);
  }
}

// ==========================================================
// שינוי BAUD לכל אחד מסוגי המודולים
// ==========================================================

void changeBaud(BluetoothModule& module)
{
  showBaudMenu();

  String selection = waitForInput();
  selection.toUpperCase();

  if (selection == "X")
  {
    Serial.println("Baud change cancelled.");
    return;
  }

  if (selection.length() != 1)
  {
    Serial.println("Invalid selection.");
    return;
  }

  char code = selection[0];
  uint32_t newBaud = baudFromCode(code);

  if (newBaud == 0)
  {
    Serial.println("Invalid selection.");
    return;
  }

  Serial.print("Selected: ");
  Serial.println(baudDescription(code));

  if (!confirmAction())
  {
    Serial.println("Baud change cancelled.");
    return;
  }

  String command =
    "AT+BAUD" + String(code);

  /*
    command examples:
    AT+BAUD1  // 1200 baud
    AT+BAUD2  // 2400 baud
    AT+BAUD3  // 4800 baud
    AT+BAUD4  // 9600 baud
    AT+BAUD5  // 19200 baud
    AT+BAUD6  // 38400 baud
    AT+BAUD7  // 57600 baud
    AT+BAUD8  // 115200 baud
  */

  Serial.print("Command: ");
  Serial.println(command);

  String response = sendDetectedCommand(
    module,
    command,
    1400,
    180
  );

  Serial.print("Response at old baud: ");

  if (response.length() == 0)
  {
    Serial.println("<no readable response>");
  }
  else
  {
    Serial.println(makeReadable(response));
  }

  delay(1500);

  openModuleUart(module, newBaud);
  module.baud = newBaud;

  bool verified = false;

  for (int attempt = 1;
       attempt <= 3;
       attempt++)
  {
    String verification = sendCommand(
      module,
      "AT",
      module.ending,
      1000,
      150
    );

    Serial.print("Verification attempt ");
    Serial.print(attempt);
    Serial.print(": ");

    if (verification.length() == 0)
    {
      Serial.println("<no response>");
    }
    else
    {
      Serial.println(makeReadable(verification));
    }

    String upper = verification;
    upper.toUpperCase();

    if (upper.indexOf("OK") >= 0)
    {
      verified = true;
      break;
    }

    delay(700);
  }

  if (verified)
  {
    Serial.println("Baud-rate change verified.");
  }
  else
  {
    Serial.println(
      "The new baud could not be verified."
    );
    Serial.println(
      "Return to the main menu and scan both modules again."
    );
  }
}

// Forward declaration: used by the module-specific menus.
void manualTerminal(BluetoothModule& module);

// ==========================================================
// תפריט HC-06 מלא
// ==========================================================

void showHc06Menu(const BluetoothModule& module)
{
  Serial.println();
  Serial.println("======================================");
  Serial.print(" HC-06 configuration - Module ");
  Serial.println(module.label);
  Serial.println("======================================");
  Serial.println("1 - Test communication");
  Serial.println("2 - Read firmware version");
  Serial.println("3 - Change device name");
  Serial.println("4 - Change pairing PIN");
  Serial.println("5 - Change UART baud rate");
  Serial.println("6 - Set parity to None");
  Serial.println("7 - Set parity to Even");
  Serial.println("8 - Set parity to Odd");
  Serial.println("M - Manual AT terminal");
  Serial.println("Q - Return to main menu");
  Serial.println("? - Show this menu");
  Serial.println();
  Serial.println(
    "The program sends None to this HC-06 automatically."
  );
  Serial.println("======================================");
}

void changeHc06Name(BluetoothModule& module)
{
  Serial.println("Enter the new name (1-20 characters):");
  String value = waitForInput();

  if (value.length() < 1 ||
      value.length() > 20)
  {
    Serial.println("Invalid name length.");
    return;
  }

  sendConfirmed(
    module,
    "AT+NAME" + value,
    ""
  );
}

void changeHc06Pin(BluetoothModule& module)
{
  Serial.println("Enter a new four-digit PIN:");
  String value = waitForInput();

  if (value.length() != 4)
  {
    Serial.println(
      "The PIN must contain exactly four digits."
    );
    return;
  }

  for (size_t i = 0; i < value.length(); i++)
  {
    if (!isDigit(value[i]))
    {
      Serial.println(
        "The PIN may contain digits only."
      );
      return;
    }
  }

  sendConfirmed(
    module,
    "AT+PIN" + value,
    ""
  );
}

void setHc06Parity(
  BluetoothModule& module,
  const String& command,
  const String& description
)
{
  Serial.print("Requested parity: ");
  Serial.println(description);

  sendConfirmed(
    module,
    command,
    ""
  );

  if (description != "None")
  {
    Serial.println(
      "Warning: this program still uses SERIAL_8N1."
    );
    Serial.println(
      "Communication may stop until the UART format is changed."
    );
  }
}

void runHc06Menu(BluetoothModule& module)
{
  showHc06Menu(module);

  while (true)
  {
    String choice = waitForInput();

    if (choice == "1")
    {
      executeAndDisplay(module, "AT");
    }
    else if (choice == "2")
    {
      executeAndDisplay(module, "AT+VERSION");
    }
    else if (choice == "3")
    {
      changeHc06Name(module);
    }
    else if (choice == "4")
    {
      changeHc06Pin(module);
    }
    else if (choice == "5")
    {
      changeBaud(module);
    }
    else if (choice == "6")
    {
      setHc06Parity(
        module,
        "AT+PN",
        "None"
      );
    }
    else if (choice == "7")
    {
      setHc06Parity(
        module,
        "AT+PE",
        "Even"
      );
    }
    else if (choice == "8")
    {
      setHc06Parity(
        module,
        "AT+PO",
        "Odd"
      );
    }
    else if (choice.equalsIgnoreCase("M"))
    {
      manualTerminal(module);
      showHc06Menu(module);
    }
    else if (choice.equalsIgnoreCase("Q"))
    {
      return;
    }
    else if (choice == "?")
    {
      showHc06Menu(module);
    }
    else
    {
      Serial.println("Unknown HC-06 menu selection.");
      showHc06Menu(module);
    }
  }
}


// ==========================================================
// תפריט HC-05 מלא
// ==========================================================

void showHc05Menu(const BluetoothModule& module)
{
  Serial.println();
  Serial.println("======================================");
  Serial.print(" HC-05 full AT configuration - Module ");
  Serial.println(module.label);
  Serial.println("======================================");

  Serial.println("READ");
  Serial.println("1 - Test communication");
  Serial.println("2 - Read firmware version");
  Serial.println("3 - Read device name");
  Serial.println("4 - Read pairing password");
  Serial.println("5 - Read UART data-mode setting");
  Serial.println("6 - Read ROLE");
  Serial.println("7 - Read connection mode CMODE");
  Serial.println("8 - Read bound address");
  Serial.println("9 - Read local Bluetooth address");

  Serial.println();
  Serial.println("CHANGE");
  Serial.println("N - Change device name");
  Serial.println("P - Change pairing password");
  Serial.println("U - Change UART data-mode setting");
  Serial.println("R - Change ROLE");
  Serial.println("C - Change CMODE");
  Serial.println("B - Change BIND address");

  Serial.println();
  Serial.println("PAIR / LINK");
  Serial.println("I - Send AT+INIT");
  Serial.println("S - Start inquiry AT+INQ");
  Serial.println("K - Pair with address");
  Serial.println("L - Link to address");

  Serial.println();
  Serial.println("OTHER");
  Serial.println("M - Manual AT terminal");
  Serial.println("Q - Return to main menu");
  Serial.println("? - Show this menu");

  Serial.println();
  Serial.println("You are now inside the HC-05 menu.");
  Serial.println("Here N always means Change device name.");
  Serial.println("HC-05 full AT mode normally uses 38400 baud and CRLF.");
  Serial.println(
    "AT+UART changes the normal data-mode UART, not the 38400 full-AT rate."
  );
  Serial.println("======================================");
}

bool validHc05Address(String address)
{
  address.toUpperCase();

  // Expected HC-05 command form: NAP,UAP,LAP, for example 1234,56,ABCDEF
  int firstComma = address.indexOf(',');
  int secondComma = address.indexOf(',', firstComma + 1);

  if (firstComma <= 0 ||
      secondComma <= firstComma + 1 ||
      secondComma >= static_cast<int>(address.length()) - 1)
  {
    return false;
  }

  for (size_t i = 0; i < address.length(); i++)
  {
    char c = address[i];

    if (c == ',')
    {
      continue;
    }

    if (!isDigit(c) &&
        !(c >= 'A' && c <= 'F'))
    {
      return false;
    }
  }

  return true;
}

String readHc05Name(BluetoothModule& module)
{
  // AT+NAME? is the documented read command for most HC-05 firmware.
  // A longer timeout is intentional because some old firmware versions
  // answer this command much more slowly than the other AT commands.
  String response = executeAndDisplay(
    module,
    "AT+NAME?",
    HC05_NAME_TIMEOUT_MS,
    HC05_NAME_QUIET_TIME_MS
  );

  if (response.length() > 0)
  {
    return response;
  }

  Serial.println(
    "AT+NAME? was not answered. Trying the legacy AT+NAME form..."
  );

  // Some older or non-standard HC-05 firmware uses AT+NAME without '?'.
  return executeAndDisplay(
    module,
    "AT+NAME",
    HC05_NAME_TIMEOUT_MS,
    HC05_NAME_QUIET_TIME_MS
  );
}

void changeHc05Name(BluetoothModule& module)
{
  Serial.println("Enter the new HC-05 name (1-20 characters):");
  String value = waitForInput();

  if (value.length() < 1 ||
      value.length() > 20)
  {
    Serial.println("Invalid name length.");
    return;
  }

  Serial.println();
  Serial.print("Command to send to Module ");
  Serial.print(module.label);
  Serial.print(": AT+NAME=");
  Serial.println(value);

  if (!confirmAction())
  {
    Serial.println("Change cancelled.");
    return;
  }

  String response = executeAndDisplay(
    module,
    "AT+NAME=" + value,
    HC05_NAME_TIMEOUT_MS,
    HC05_NAME_QUIET_TIME_MS
  );

  if (!successfulResponse(response))
  {
    Serial.println(
      "The module did not clearly confirm the name change."
    );
  }

  delay(1200);

  Serial.println("Reading the device name back for verification...");
  String readBack = readHc05Name(module);

  if (readBack.length() == 0)
  {
    Serial.println(
      "The HC-05 did not support name read-back. "
      "The write command may still have succeeded."
    );
  }
}

void changeHc05Password(BluetoothModule& module)
{
  Serial.println("Enter a new four-digit pairing password:");
  String value = waitForInput();

  if (value.length() != 4)
  {
    Serial.println("The password must contain exactly four digits.");
    return;
  }

  for (size_t i = 0; i < value.length(); i++)
  {
    if (!isDigit(value[i]))
    {
      Serial.println("The password may contain digits only.");
      return;
    }
  }

  sendConfirmed(
    module,
    "AT+PSWD=" + value,
    "AT+PSWD?"
  );
}

void changeHc05Uart(BluetoothModule& module)
{
  Serial.println("Select the HC-05 normal data-mode UART baud:");
  Serial.println("1 - 1200 baud");
  Serial.println("2 - 2400 baud");
  Serial.println("3 - 4800 baud");
  Serial.println("4 - 9600 baud");
  Serial.println("5 - 19200 baud");
  Serial.println("6 - 38400 baud");
  Serial.println("7 - 57600 baud");
  Serial.println("8 - 115200 baud");
  Serial.println("X - Cancel");

  String selection = waitForInput();
  selection.toUpperCase();

  if (selection == "X")
  {
    Serial.println("UART change cancelled.");
    return;
  }

  if (selection.length() != 1)
  {
    Serial.println("Invalid selection.");
    return;
  }

  uint32_t selectedBaud =
    baudFromCode(selection[0]);

  if (selectedBaud == 0)
  {
    Serial.println("Invalid selection.");
    return;
  }

  String command =
    "AT+UART=" + String(selectedBaud) + ",0,0";

  Serial.print("New data-mode UART: ");
  Serial.println(selectedBaud);

  sendConfirmed(
    module,
    command,
    "AT+UART?"
  );

  Serial.println(
    "The HC-05 remains at 38400 while it stays in full AT mode."
  );
}

void changeHc05Role(BluetoothModule& module)
{
  Serial.println("Select ROLE:");
  Serial.println("0 - Slave");
  Serial.println("1 - Master");

  String value = waitForInput();

  if (value != "0" &&
      value != "1")
  {
    Serial.println("Invalid ROLE value.");
    return;
  }

  sendConfirmed(
    module,
    "AT+ROLE=" + value,
    "AT+ROLE?"
  );
}

void changeHc05Cmode(BluetoothModule& module)
{
  Serial.println("Select CMODE:");
  Serial.println("0 - Connect only to the bound address");
  Serial.println("1 - Connect to any address");

  String value = waitForInput();

  if (value != "0" &&
      value != "1")
  {
    Serial.println("Invalid CMODE value.");
    return;
  }

  sendConfirmed(
    module,
    "AT+CMODE=" + value,
    "AT+CMODE?"
  );
}

String requestHc05Address()
{
  Serial.println(
    "Enter address in HC-05 format: NAP,UAP,LAP"
  );
  Serial.println("Example: 1234,56,ABCDEF");

  String address = waitForInput();
  address.toUpperCase();

  if (!validHc05Address(address))
  {
    Serial.println("Invalid HC-05 address format.");
    return "";
  }

  return address;
}

void changeHc05Bind(BluetoothModule& module)
{
  String address = requestHc05Address();

  if (address.length() == 0)
  {
    return;
  }

  sendConfirmed(
    module,
    "AT+BIND=" + address,
    "AT+BIND?"
  );
}

void hc05Inquiry(BluetoothModule& module)
{
  Serial.println();
  Serial.println("Starting HC-05 inquiry for 20 seconds...");

  clearModuleInput(module);

  module.uart->print("AT+INQ");
  module.uart->print(endingText(module.ending));
  module.uart->flush();

  unsigned long startTime = millis();

  while (millis() - startTime < 20000)
  {
    while (module.uart->available())
    {
      Serial.write(module.uart->read());
    }

    delay(1);
  }

  Serial.println();
  Serial.println("HC-05 inquiry listening period finished.");
}

void hc05Pair(BluetoothModule& module)
{
  String address = requestHc05Address();

  if (address.length() == 0)
  {
    return;
  }

  Serial.println("Enter timeout value, for example 20:");
  String timeoutValue = waitForInput();

  if (timeoutValue.length() == 0)
  {
    Serial.println("Invalid timeout.");
    return;
  }

  sendConfirmed(
    module,
    "AT+PAIR=" + address + "," + timeoutValue,
    ""
  );
}

void hc05Link(BluetoothModule& module)
{
  String address = requestHc05Address();

  if (address.length() == 0)
  {
    return;
  }

  sendConfirmed(
    module,
    "AT+LINK=" + address,
    ""
  );
}

void runHc05Menu(BluetoothModule& module)
{
  showHc05Menu(module);

  while (true)
  {
    Serial.print("HC-05 Module ");
    Serial.print(module.label);
    Serial.print(" menu > ");

    String choice = waitForInput();

    if (choice == "1")
    {
      executeAndDisplay(module, "AT");
    }
    else if (choice == "2")
    {
      executeAndDisplay(module, "AT+VERSION?");
    }
    else if (choice == "3")
    {
      readHc05Name(module);
    }
    else if (choice == "4")
    {
      executeAndDisplay(module, "AT+PSWD?");
    }
    else if (choice == "5")
    {
      executeAndDisplay(module, "AT+UART?");
    }
    else if (choice == "6")
    {
      executeAndDisplay(module, "AT+ROLE?");
    }
    else if (choice == "7")
    {
      executeAndDisplay(module, "AT+CMODE?");
    }
    else if (choice == "8")
    {
      executeAndDisplay(module, "AT+BIND?");
    }
    else if (choice == "9")
    {
      executeAndDisplay(module, "AT+ADDR?");
    }
    else if (choice.equalsIgnoreCase("N"))
    {
      changeHc05Name(module);
    }
    else if (choice.equalsIgnoreCase("P"))
    {
      changeHc05Password(module);
    }
    else if (choice.equalsIgnoreCase("U"))
    {
      changeHc05Uart(module);
    }
    else if (choice.equalsIgnoreCase("R"))
    {
      changeHc05Role(module);
    }
    else if (choice.equalsIgnoreCase("C"))
    {
      changeHc05Cmode(module);
    }
    else if (choice.equalsIgnoreCase("B"))
    {
      changeHc05Bind(module);
    }
    else if (choice.equalsIgnoreCase("I"))
    {
      executeAndDisplay(module, "AT+INIT");
    }
    else if (choice.equalsIgnoreCase("S"))
    {
      hc05Inquiry(module);
    }
    else if (choice.equalsIgnoreCase("K"))
    {
      hc05Pair(module);
    }
    else if (choice.equalsIgnoreCase("L"))
    {
      hc05Link(module);
    }
    else if (choice.equalsIgnoreCase("M"))
    {
      manualTerminal(module);
      showHc05Menu(module);
    }
    else if (choice.equalsIgnoreCase("Q"))
    {
      return;
    }
    else if (choice == "?")
    {
      showHc05Menu(module);
    }
    else
    {
      Serial.println("Unknown HC-05 menu selection.");
      showHc05Menu(module);
    }
  }
}

// ==========================================================
// פונקציות HM-10 Clone
// ==========================================================

void explainHm10Type(const String& response)
{
  if (response.indexOf("+TYPE=0") >= 0)
  {
    Serial.println("TYPE=0: No PIN authentication.");
  }
  else if (response.indexOf("+TYPE=1") >= 0)
  {
    Serial.println("TYPE=1: Authentication mode 1.");
  }
  else if (response.indexOf("+TYPE=2") >= 0)
  {
    Serial.println("TYPE=2: Authentication mode 2.");
  }
  else if (response.indexOf("+TYPE=3") >= 0)
  {
    Serial.println("TYPE=3: Authentication mode 3.");
  }
}

void explainHm10Role(const String& response)
{
  if (response.indexOf("+ROLE=0") >= 0)
  {
    Serial.println("ROLE=0: Peripheral / Slave.");
  }
  else if (response.indexOf("+ROLE=1") >= 0)
  {
    Serial.println("ROLE=1: Central / Master.");
  }
}

void explainHm10Power(const String& response)
{
  if (response.indexOf("+POWE=0") >= 0)
  {
    Serial.println("POWE=0: Lowest RF power.");
  }
  else if (response.indexOf("+POWE=1") >= 0)
  {
    Serial.println("POWE=1: Low RF power.");
  }
  else if (response.indexOf("+POWE=2") >= 0)
  {
    Serial.println("POWE=2: Medium RF power.");
  }
  else if (response.indexOf("+POWE=3") >= 0)
  {
    Serial.println("POWE=3: Highest RF power.");
  }
}

bool isFourDigitHex(String value)
{
  value.toUpperCase();

  if (value.length() != 4)
  {
    return false;
  }

  for (size_t i = 0; i < value.length(); i++)
  {
    char c = value[i];

    if (!isDigit(c) &&
        !(c >= 'A' && c <= 'F'))
    {
      return false;
    }
  }

  return true;
}

void readAllHm10(BluetoothModule& module)
{
  executeAndDisplay(module, "AT");
  executeAndDisplay(module, "AT+VERSION");
  executeAndDisplay(module, "AT+NAME");
  executeAndDisplay(module, "AT+PIN");

  String baudResponse =
    executeAndDisplay(module, "AT+BAUD");

  if (baudResponse.indexOf("+BAUD=") >= 0)
  {
    explainBaudResponse(baudResponse);
  }

  String typeResponse =
    executeAndDisplay(module, "AT+TYPE");
  explainHm10Type(typeResponse);

  String roleResponse =
    executeAndDisplay(module, "AT+ROLE");
  explainHm10Role(roleResponse);

  String powerResponse =
    executeAndDisplay(module, "AT+POWE");
  explainHm10Power(powerResponse);

  executeAndDisplay(module, "AT+UUID");
  executeAndDisplay(module, "AT+CHAR");
  executeAndDisplay(module, "AT+LADDR");
  executeAndDisplay(module, "AT+IMME?");
  executeAndDisplay(module, "AT+PARI?");
  executeAndDisplay(module, "AT+STOP?");
}

void changeHm10Name(BluetoothModule& module)
{
  Serial.println("Enter the new name (1-20 characters):");
  String value = waitForInput();

  if (value.length() < 1 ||
      value.length() > 20)
  {
    Serial.println("Invalid name length.");
    return;
  }

  sendConfirmed(
    module,
    "AT+NAME" + value,
    "AT+NAME"
  );
}

void changeHm10Pin(BluetoothModule& module)
{
  Serial.println("Enter the new six-digit PIN:");
  String value = waitForInput();

  if (value.length() != 6)
  {
    Serial.println(
      "The PIN must contain exactly six digits."
    );
    return;
  }

  for (size_t i = 0; i < value.length(); i++)
  {
    if (!isDigit(value[i]))
    {
      Serial.println(
        "The PIN may contain digits only."
      );
      return;
    }
  }

  sendConfirmed(
    module,
    "AT+PIN" + value,
    "AT+PIN"
  );
}

void changeHm10Type(BluetoothModule& module)
{
  Serial.println("Enter TYPE value 0-3:");
  String value = waitForInput();

  if (value.length() != 1 ||
      value[0] < '0' ||
      value[0] > '3')
  {
    Serial.println("Invalid TYPE value.");
    return;
  }

  sendConfirmed(
    module,
    "AT+TYPE" + value,
    "AT+TYPE"
  );
}

void changeHm10Role(BluetoothModule& module)
{
  Serial.println("Select ROLE:");
  Serial.println("0 - Peripheral / Slave");
  Serial.println("1 - Central / Master");

  String value = waitForInput();

  if (value != "0" &&
      value != "1")
  {
    Serial.println("Invalid ROLE value.");
    return;
  }

  sendConfirmed(
    module,
    "AT+ROLE" + value,
    "AT+ROLE"
  );
}

void changeHm10Power(BluetoothModule& module)
{
  Serial.println("Select RF power code 0-3:");
  Serial.println("0 - Lowest");
  Serial.println("1 - Low");
  Serial.println("2 - Medium");
  Serial.println("3 - Highest");

  String value = waitForInput();

  if (value.length() != 1 ||
      value[0] < '0' ||
      value[0] > '3')
  {
    Serial.println("Invalid power code.");
    return;
  }

  sendConfirmed(
    module,
    "AT+POWE" + value,
    "AT+POWE"
  );
}

void changeHm10Uuid(BluetoothModule& module)
{
  Serial.println(
    "Enter a four-digit hexadecimal service UUID."
  );
  Serial.println("Example: FFE0");

  String value = waitForInput();
  value.toUpperCase();

  if (!isFourDigitHex(value))
  {
    Serial.println("Invalid UUID.");
    return;
  }

  Serial.println(
    "Warning: changing UUID can prevent apps from finding the service."
  );

  sendConfirmed(
    module,
    "AT+UUID" + value,
    "AT+UUID"
  );
}

void changeHm10Char(BluetoothModule& module)
{
  Serial.println(
    "Enter a four-digit hexadecimal characteristic UUID."
  );
  Serial.println("Example: FFE1");

  String value = waitForInput();
  value.toUpperCase();

  if (!isFourDigitHex(value))
  {
    Serial.println("Invalid characteristic UUID.");
    return;
  }

  Serial.println(
    "Warning: changing CHAR can prevent apps from exchanging data."
  );

  sendConfirmed(
    module,
    "AT+CHAR" + value,
    "AT+CHAR"
  );
}

void setHm10Imme(BluetoothModule& module)
{
  Serial.println("Select IMME:");
  Serial.println("0 - Automatic start after power-up");
  Serial.println("1 - Wait for AT+START");

  String value = waitForInput();

  if (value != "0" &&
      value != "1")
  {
    Serial.println("Invalid IMME value.");
    return;
  }

  sendConfirmed(
    module,
    "AT+IMME" + value,
    "AT+IMME?"
  );
}

void setHm10ParitySafe(BluetoothModule& module)
{
  Serial.println(
    "This safe option writes PARI=0 only."
  );
  Serial.println(
    "PARI=0 means no parity and keeps SERIAL_8N1 compatible."
  );

  sendConfirmed(
    module,
    "AT+PARI0",
    "AT+PARI?"
  );
}

void setHm10StopSafe(BluetoothModule& module)
{
  Serial.println(
    "This safe option writes STOP=0 only."
  );
  Serial.println(
    "STOP=0 means one stop bit and keeps SERIAL_8N1 compatible."
  );

  sendConfirmed(
    module,
    "AT+STOP0",
    "AT+STOP?"
  );
}

// ==========================================================
// תפריט מתקדם HM-10
// ==========================================================

void showHm10AdvancedMenu(
  const BluetoothModule& module
)
{
  Serial.println();
  Serial.println("======================================");
  Serial.print(" HM-10 advanced menu - Module ");
  Serial.println(module.label);
  Serial.println("======================================");

  Serial.println("READ");
  Serial.println("1 - Read IMME using AT+IMME?");
  Serial.println("2 - Read UART parity using AT+PARI?");
  Serial.println("3 - Read UART stop bits using AT+STOP?");
  Serial.println("4 - Test STATE? (unsupported on tested firmware)");
  Serial.println("5 - Test PWRM? (unsupported on tested v6.1)");

  Serial.println();
  Serial.println("CHANGE");
  Serial.println("I - Set IMME to 0 or 1");
  Serial.println("P - Safely set parity to 0 / None");
  Serial.println("T - Safely set stop bits to 0 / one stop bit");

  Serial.println();
  Serial.println("MASTER TESTS");
  Serial.println("7 - Start 30-second BLE inquiry");
  Serial.println("8 - Send AT+SHOW");
  Serial.println("9 - Send AT+SHOW?");

  Serial.println();
  Serial.println("Q - Return to main HM-10 menu");
  Serial.println("? - Show this advanced menu");

  Serial.println();
  Serial.println(
    "SLEEP, START, CONN, DEFAULT and RENEW are not run automatically."
  );
  Serial.println("======================================");
}

void runLongInquiry(BluetoothModule& module)
{
  Serial.println();
  Serial.println("======================================");
  Serial.print(" Long BLE inquiry - Module ");
  Serial.println(module.label);
  Serial.println("======================================");

  clearModuleInput(module);

  module.uart->print("AT+INQ");
  module.uart->print(endingText(module.ending));
  module.uart->flush();

  unsigned long startTime = millis();

  while (millis() - startTime <
         INQUIRY_LISTEN_MS)
  {
    while (module.uart->available())
    {
      Serial.write(module.uart->read());
    }

    delay(1);
  }

  Serial.println();
  Serial.println("Inquiry listening period finished.");
}

void runHm10AdvancedMenu(
  BluetoothModule& module
)
{
  showHm10AdvancedMenu(module);

  while (true)
  {
    String choice = waitForInput();

    if (choice == "1")
    {
      executeAndDisplay(module, "AT+IMME?");
    }
    else if (choice == "2")
    {
      executeAndDisplay(module, "AT+PARI?");
    }
    else if (choice == "3")
    {
      executeAndDisplay(module, "AT+STOP?");
    }
    else if (choice == "4")
    {
      executeAndDisplay(module, "AT+STATE?");
    }
    else if (choice == "5")
    {
      executeAndDisplay(module, "AT+PWRM?");
    }
    else if (choice.equalsIgnoreCase("I"))
    {
      setHm10Imme(module);
    }
    else if (choice.equalsIgnoreCase("P"))
    {
      setHm10ParitySafe(module);
    }
    else if (choice.equalsIgnoreCase("T"))
    {
      setHm10StopSafe(module);
    }
    else if (choice == "7")
    {
      runLongInquiry(module);
    }
    else if (choice == "8")
    {
      executeAndDisplay(module, "AT+SHOW");
    }
    else if (choice == "9")
    {
      executeAndDisplay(module, "AT+SHOW?");
    }
    else if (choice.equalsIgnoreCase("Q"))
    {
      return;
    }
    else if (choice == "?")
    {
      showHm10AdvancedMenu(module);
    }
    else
    {
      Serial.println("Unknown advanced-menu selection.");
      showHm10AdvancedMenu(module);
    }
  }
}

// ==========================================================
// תפריט HM-10 מלא
// ==========================================================

void showHm10Menu(const BluetoothModule& module)
{
  Serial.println();
  Serial.println("======================================");
  Serial.print(" HM-10 Clone configuration - Module ");
  Serial.println(module.label);
  Serial.println("======================================");

  Serial.println("READ");
  Serial.println("1 - Test communication");
  Serial.println("2 - Read firmware version");
  Serial.println("3 - Read device name");
  Serial.println("4 - Read pairing PIN");
  Serial.println("5 - Read UART baud setting");
  Serial.println("6 - Read TYPE");
  Serial.println("7 - Read ROLE");
  Serial.println("8 - Read RF power");
  Serial.println("9 - Read UUID and characteristic");
  Serial.println("L - Read local Bluetooth address");
  Serial.println("A - Read all known parameters");

  Serial.println();
  Serial.println("CHANGE");
  Serial.println("N - Change device name");
  Serial.println("P - Change pairing PIN");
  Serial.println("B - Change UART baud rate");
  Serial.println("T - Change TYPE");
  Serial.println("R - Change ROLE");
  Serial.println("W - Change RF power");
  Serial.println("U - Change service UUID");
  Serial.println("C - Change characteristic UUID");

  Serial.println();
  Serial.println("OTHER");
  Serial.println("X - Open advanced menu");
  Serial.println("H - Show AT command HELP");
  Serial.println("M - Manual AT terminal");
  Serial.println("Q - Return to main menu");
  Serial.println("? - Show this menu");

  Serial.println();
  Serial.println(
    "The program sends CRLF to this module automatically."
  );
  Serial.println("======================================");
}

void runHm10Menu(BluetoothModule& module)
{
  showHm10Menu(module);

  while (true)
  {
    String choice = waitForInput();

    if (choice == "1")
    {
      executeAndDisplay(module, "AT");
    }
    else if (choice == "2")
    {
      executeAndDisplay(module, "AT+VERSION");
    }
    else if (choice == "3")
    {
      executeAndDisplay(module, "AT+NAME");
    }
    else if (choice == "4")
    {
      executeAndDisplay(module, "AT+PIN");
    }
    else if (choice == "5")
    {
      String response =
        executeAndDisplay(module, "AT+BAUD");

      if (response.indexOf("+BAUD=") >= 0)
      {
        explainBaudResponse(response);
      }
    }
    else if (choice == "6")
    {
      String response =
        executeAndDisplay(module, "AT+TYPE");
      explainHm10Type(response);
    }
    else if (choice == "7")
    {
      String response =
        executeAndDisplay(module, "AT+ROLE");
      explainHm10Role(response);
    }
    else if (choice == "8")
    {
      String response =
        executeAndDisplay(module, "AT+POWE");
      explainHm10Power(response);
    }
    else if (choice == "9")
    {
      executeAndDisplay(module, "AT+UUID");
      executeAndDisplay(module, "AT+CHAR");
    }
    else if (choice.equalsIgnoreCase("L"))
    {
      executeAndDisplay(module, "AT+LADDR");
    }
    else if (choice.equalsIgnoreCase("A"))
    {
      readAllHm10(module);
    }
    else if (choice.equalsIgnoreCase("N"))
    {
      changeHm10Name(module);
    }
    else if (choice.equalsIgnoreCase("P"))
    {
      changeHm10Pin(module);
    }
    else if (choice.equalsIgnoreCase("B"))
    {
      changeBaud(module);
    }
    else if (choice.equalsIgnoreCase("T"))
    {
      changeHm10Type(module);
    }
    else if (choice.equalsIgnoreCase("R"))
    {
      changeHm10Role(module);
    }
    else if (choice.equalsIgnoreCase("W"))
    {
      changeHm10Power(module);
    }
    else if (choice.equalsIgnoreCase("U"))
    {
      changeHm10Uuid(module);
    }
    else if (choice.equalsIgnoreCase("C"))
    {
      changeHm10Char(module);
    }
    else if (choice.equalsIgnoreCase("X"))
    {
      runHm10AdvancedMenu(module);
      showHm10Menu(module);
    }
    else if (choice.equalsIgnoreCase("H"))
    {
      executeAndDisplay(
        module,
        "AT+HELP",
        HELP_TIMEOUT_MS,
        HELP_QUIET_TIME_MS
      );
    }
    else if (choice.equalsIgnoreCase("M"))
    {
      manualTerminal(module);
      showHm10Menu(module);
    }
    else if (choice.equalsIgnoreCase("Q"))
    {
      return;
    }
    else if (choice == "?")
    {
      showHm10Menu(module);
    }
    else
    {
      Serial.println("Unknown HM-10 menu selection.");
      showHm10Menu(module);
    }
  }
}


// ==========================================================
// HMSoft V006 support
// ==========================================================

int extractHmSoftGetCode(const String& response)
{
  int marker = response.indexOf("OK+Get:");

  if (marker < 0)
  {
    return -1;
  }

  int position = marker + 7;

  if (position >= static_cast<int>(response.length()))
  {
    return -1;
  }

  char value = response[position];

  if (value < '0' || value > '9')
  {
    return -1;
  }

  return value - '0';
}

unsigned long hmSoftProbeTimeoutMs(uint32_t baud)
{
  if (baud <= 1200)   return 2200;
  if (baud <= 2400)   return 1600;
  if (baud <= 4800)   return 1200;
  if (baud <= 9600)   return 900;
  if (baud <= 19200)  return 750;
  if (baud <= 38400)  return 650;
  return 550;
}

unsigned long hmSoftQuietTimeMs(uint32_t baud)
{
  if (baud <= 1200)   return 260;
  if (baud <= 2400)   return 220;
  if (baud <= 4800)   return 180;
  if (baud <= 9600)   return 150;
  return 110;
}

bool probeHmSoftAtBaud(
  BluetoothModule& module,
  uint32_t candidate,
  uint8_t attempts = 3
)
{
  openModuleUart(module, candidate);
  delay(candidate <= 2400 ? 350 : 180);

  for (uint8_t attempt = 1; attempt <= attempts; attempt++)
  {
    String response = sendCommand(
      module,
      "AT",
      LineEnding::NONE,
      hmSoftProbeTimeoutMs(candidate),
      hmSoftQuietTimeMs(candidate)
    );

    String upper = response;
    upper.toUpperCase();

    if (upper.indexOf("OK") >= 0)
    {
      module.baud = candidate;
      module.ending = LineEnding::NONE;
      return true;
    }

    delay(candidate <= 2400 ? 250 : 100);
  }

  return false;
}

bool findCurrentBaudAfterChange(
  BluetoothModule& module,
  uint32_t& foundBaud,
  bool showProgress = false
)
{
  for (size_t i = 0; i < BAUD_COUNT; i++)
  {
    uint32_t candidate = BAUD_RATES[i];

    if (showProgress)
    {
      Serial.print("  Recovery probe at ");
      Serial.print(candidate);
      Serial.println(" baud...");
    }

    if (probeHmSoftAtBaud(module, candidate))
    {
      foundBaud = candidate;

      if (showProgress)
      {
        Serial.print("  HMSoft response found at ");
        Serial.print(candidate);
        Serial.println(" baud.");
      }

      return true;
    }
  }

  foundBaud = 0;
  return false;
}

void mapHmSoftBaudCodes(BluetoothModule& module)
{
  Serial.println();
  Serial.println("======================================");
  Serial.print(" Safe HMSoft baud-code mapper - Module ");
  Serial.println(module.label);
  Serial.println("======================================");
  Serial.println("This tool maps only the baud codes that were");
  Serial.println("shown to be recoverable in the laboratory.");
  Serial.println();
  Serial.println("Safe test order: 0, 1, 2, 3, 4, 5, 6, 8");
  Serial.println("Code 7 is BLOCKED and will not be sent.");
  Serial.println("Reason: on HMSoft V006, the tested module became");
  Serial.println("unreachable after code 7 was selected.");
  Serial.println();
  Serial.println("The original baud code is restored at the end.");
  Serial.println("Do not disconnect power while this test is running.");
  Serial.println();
  Serial.println("POWER SAFETY REMINDER:");
  Serial.println("HM-10 Clone / HMSoft BT4.0 VCC = 3.3V from ESP32.");
  Serial.println("Do not power this BLE module from 5V.");
  Serial.println();

  if (!confirmAction())
  {
    Serial.println("Baud mapping cancelled.");
    return;
  }

  String originalResponse = executeAndDisplay(module, "AT+BAUD?");
  int originalCode = extractHmSoftGetCode(originalResponse);
  uint32_t originalBaud = module.baud;

  if (originalCode < 0)
  {
    Serial.println("Could not read the original HMSoft baud code.");
    Serial.println("The mapper was stopped without changing the module.");
    return;
  }

  Serial.println();
  Serial.print("Original code: ");
  Serial.println(originalCode);
  Serial.print("Original detected baud: ");
  Serial.println(originalBaud);

  int mappedCodes[9];
  uint32_t mappedBauds[9];

  for (int i = 0; i < 9; i++)
  {
    mappedCodes[i] = i;
    mappedBauds[i] = 0;
  }

  // Code 7 is intentionally excluded. It is not sent to the module.
  const int safeCodeOrder[] = {0, 1, 2, 3, 4, 5, 6, 8};
  const size_t safeCodeCount =
    sizeof(safeCodeOrder) / sizeof(safeCodeOrder[0]);

  uint32_t lastKnownBaud = originalBaud;
  bool stoppedEarly = false;

  for (size_t index = 0; index < safeCodeCount; index++)
  {
    int code = safeCodeOrder[index];

    Serial.println();
    Serial.println("--------------------------------------");
    Serial.print("Testing safe HMSoft baud code ");
    Serial.println(code);
    Serial.println("--------------------------------------");

    openModuleUart(module, lastKnownBaud);
    delay(300);

    // Dynamic command: AT+BAUD<code>. Code 7 is never generated here.
    // The discovered numeric baud rate is printed immediately afterward.
    String command = "AT+BAUD" + String(code);

    String response = sendCommand(
      module,
      command,
      LineEnding::NONE,
      hmSoftProbeTimeoutMs(lastKnownBaud),
      hmSoftQuietTimeMs(lastKnownBaud)
    );

    Serial.print("Change response: ");
    Serial.println(
      response.length() > 0
        ? makeReadable(response)
        : "<no response>"
    );

    // Allow the firmware enough time to store and apply the new setting.
    delay(1500);

    uint32_t discoveredBaud = 0;

    if (!findCurrentBaudAfterChange(module, discoveredBaud, true))
    {
      Serial.println("The module was not found after this safe code change.");
      Serial.println("Mapping stops to avoid any additional changes.");
      stoppedEarly = true;
      break;
    }

    mappedBauds[code] = discoveredBaud;
    lastKnownBaud = discoveredBaud;

    Serial.print("Mapped result: code ");
    Serial.print(code);
    Serial.print(" -> ");
    Serial.print(discoveredBaud);
    Serial.println(" baud");

    String verify = sendCommand(
      module,
      "AT+BAUD?",
      LineEnding::NONE,
      hmSoftProbeTimeoutMs(discoveredBaud),
      hmSoftQuietTimeMs(discoveredBaud)
    );

    Serial.print("Read-back: ");
    Serial.println(
      verify.length() > 0
        ? makeReadable(verify)
        : "<no response>"
    );
  }

  Serial.println();
  Serial.println("======================================");
  Serial.println(" SAFE HMSOFT BAUD MAPPING RESULTS");
  Serial.println("======================================");

  for (int code = 0; code <= 8; code++)
  {
    Serial.print("Code ");
    Serial.print(mappedCodes[code]);
    Serial.print(" -> ");

    if (code == 7)
    {
      Serial.println("BLOCKED - not tested to protect the module");
    }
    else if (mappedBauds[code] == 0)
    {
      Serial.println("not mapped");
    }
    else
    {
      Serial.print(mappedBauds[code]);
      Serial.println(" baud");
    }
  }

  Serial.println("======================================");
  Serial.println();
  Serial.print("Restoring original baud code ");
  Serial.print(originalCode);
  Serial.println("...");

  uint32_t recoveryBaud = 0;

  if (!probeHmSoftAtBaud(module, lastKnownBaud) &&
      !findCurrentBaudAfterChange(module, recoveryBaud, true))
  {
    Serial.println("Warning: the module could not be located before restoration.");
    Serial.println("Run the main scan again before other operations.");

    if (stoppedEarly)
    {
      Serial.println("The mapping table is partial because the test stopped early.");
    }

    return;
  }

  if (recoveryBaud != 0)
  {
    lastKnownBaud = recoveryBaud;
  }

  openModuleUart(module, lastKnownBaud);
  delay(lastKnownBaud <= 2400 ? 500 : 300);

  // Dynamic restoration command: AT+BAUD<originalCode>.
  // The corresponding original numeric baud is printed above.
  String restoreCommand = "AT+BAUD" + String(originalCode);
  String restoreResponse = sendCommand(
    module,
    restoreCommand,
    LineEnding::NONE,
    hmSoftProbeTimeoutMs(lastKnownBaud),
    hmSoftQuietTimeMs(lastKnownBaud)
  );

  Serial.print("Restore response: ");
  Serial.println(
    restoreResponse.length() > 0
      ? makeReadable(restoreResponse)
      : "<no response>"
  );

  delay(1500);

  uint32_t restoredBaud = 0;

  if (findCurrentBaudAfterChange(module, restoredBaud, true))
  {
    Serial.print("Module found after restoration at ");
    Serial.print(restoredBaud);
    Serial.println(" baud.");

    module.baud = restoredBaud;
    module.ending = LineEnding::NONE;
    module.detected = true;

    if (restoredBaud == originalBaud)
    {
      Serial.println("Original UART baud was restored successfully.");
    }
    else
    {
      Serial.println("Warning: restored baud differs from the original baud.");
    }
  }
  else
  {
    Serial.println("Warning: module was not found after restoration.");
    Serial.println("Run the main scan again before other operations.");
  }

  if (stoppedEarly)
  {
    Serial.println("The mapping table is partial because the test stopped early.");
  }
}

void showHmSoftMenu(const BluetoothModule& module)
{
  Serial.println();
  Serial.println("======================================");
  Serial.print(" HMSoft configuration - Module ");
  Serial.println(module.label);
  Serial.println("======================================");

  Serial.println("READ");
  Serial.println("1 - Test communication");
  Serial.println("2 - Read firmware version (AT+VERS?)");
  Serial.println("3 - Read device name");
  Serial.println("4 - Read UART baud code");
  Serial.println("5 - Read ROLE");
  Serial.println("6 - Read TYPE");
  Serial.println("7 - Read IMME");
  Serial.println("8 - Read PARI");
  Serial.println("9 - Read STOP");
  Serial.println("L - Read local Bluetooth address");
  Serial.println("A - Read all verified parameters");

  Serial.println();
  Serial.println("DIAGNOSTIC");
  Serial.println(
    "T - Automatically map HMSoft baud codes"
  );

  Serial.println();
  Serial.println("OTHER");
  Serial.println("M - Manual AT terminal");
  Serial.println("Q - Return to main menu");
  Serial.println("? - Show this menu");

  Serial.println();
  Serial.println(
    "Verified syntax: 115200 baud, no line ending,"
  );
  Serial.println(
    "AT+VERS?, AT+NAME?, AT+BAUD?, AT+ADDR?."
  );
  Serial.println("======================================");
}

void runHmSoftMenu(BluetoothModule& module)
{
  showHmSoftMenu(module);

  while (true)
  {
    String choice = waitForInput();

    if (choice == "1")
    {
      executeAndDisplay(module, "AT");
    }
    else if (choice == "2")
    {
      executeAndDisplay(module, "AT+VERS?");
    }
    else if (choice == "3")
    {
      executeAndDisplay(module, "AT+NAME?");
    }
    else if (choice == "4")
    {
      executeAndDisplay(module, "AT+BAUD?");
    }
    else if (choice == "5")
    {
      executeAndDisplay(module, "AT+ROLE?");
    }
    else if (choice == "6")
    {
      executeAndDisplay(module, "AT+TYPE?");
    }
    else if (choice == "7")
    {
      executeAndDisplay(module, "AT+IMME?");
    }
    else if (choice == "8")
    {
      executeAndDisplay(module, "AT+PARI?");
    }
    else if (choice == "9")
    {
      executeAndDisplay(module, "AT+STOP?");
    }
    else if (choice.equalsIgnoreCase("L"))
    {
      executeAndDisplay(module, "AT+ADDR?");
    }
    else if (choice.equalsIgnoreCase("A"))
    {
      readAllHm10(module);
    }
    else if (choice.equalsIgnoreCase("T"))
    {
      mapHmSoftBaudCodes(module);
      showHmSoftMenu(module);
    }
    else if (choice.equalsIgnoreCase("M"))
    {
      manualTerminal(module);
      showHmSoftMenu(module);
    }
    else if (choice.equalsIgnoreCase("Q"))
    {
      return;
    }
    else if (choice == "?")
    {
      showHmSoftMenu(module);
    }
    else
    {
      Serial.println(
        "Unknown HMSoft menu selection."
      );
      showHmSoftMenu(module);
    }
  }
}

// ==========================================================
// מסוף AT ידני לכל מודול
// ==========================================================

void manualTerminal(BluetoothModule& module)
{
  if (!module.detected)
  {
    Serial.print("Module ");
    Serial.print(module.label);
    Serial.println(" is not detected.");
    return;
  }

  Serial.println();
  Serial.println("======================================");
  Serial.print(" Manual AT terminal - Module ");
  Serial.println(module.label);
  Serial.println("======================================");
  Serial.print("Detected module line ending: ");
  Serial.println(endingName(module.ending));
  Serial.println("Type an AT command.");
  Serial.println("Type /listen for 30-second listening.");
  Serial.println("Type /exit to return.");

  while (true)
  {
    String command = waitForInput();

    if (command.equalsIgnoreCase("/exit"))
    {
      return;
    }

    if (command.equalsIgnoreCase("/listen"))
    {
      Serial.println("Listening for 30 seconds...");

      unsigned long startTime = millis();

      while (millis() - startTime <
             INQUIRY_LISTEN_MS)
      {
        while (module.uart->available())
        {
          Serial.write(module.uart->read());
        }

        delay(1);
      }

      Serial.println();
      Serial.println("Listening finished.");
      continue;
    }

    unsigned long timeoutMs =
      NORMAL_TIMEOUT_MS;

    unsigned long quietTimeMs =
      QUIET_TIME_MS;

    if (command.equalsIgnoreCase("AT+HELP"))
    {
      timeoutMs = HELP_TIMEOUT_MS;
      quietTimeMs = HELP_QUIET_TIME_MS;
    }

    executeAndDisplay(
      module,
      command,
      timeoutMs,
      quietTimeMs
    );
  }
}

// ==========================================================
// כניסה לתפריט המתאים לכל מודול
// ==========================================================

void configureModule(BluetoothModule& module)
{
  if (!module.detected)
  {
    Serial.print("Module ");
    Serial.print(module.label);
    Serial.println(" is not detected.");
    return;
  }

  if (module.type == ModuleType::HC05_FULL_AT)
  {
    runHc05Menu(module);
  }
  else if (module.type == ModuleType::HC06_LINVOR)
  {
    runHc06Menu(module);
  }
  else if (module.type == ModuleType::HM10_CLONE)
  {
    runHm10Menu(module);
  }
  else if (module.type == ModuleType::HMSOFT_V006)
  {
    runHmSoftMenu(module);
  }
  else
  {
    Serial.println(
      "Unknown module type. Opening manual terminal."
    );
    manualTerminal(module);
  }
}

// ==========================================================
// מעבדת זוג HM-10
// ==========================================================

bool requireTwoHm10Modules()
{
  if (!moduleA.detected ||
      !moduleB.detected)
  {
    Serial.println(
      "Both modules must be detected first."
    );
    return false;
  }

  if (moduleA.type != ModuleType::HM10_CLONE ||
      moduleB.type != ModuleType::HM10_CLONE)
  {
    Serial.println(
      "This operation requires two HM-10-compatible BLE modules."
    );
    return false;
  }

  return true;
}

void prepareMasterSlavePair()
{
  if (!requireTwoHm10Modules())
  {
    return;
  }

  Serial.println();
  Serial.println("======================================");
  Serial.println(" Preparing BLE pair");
  Serial.println("======================================");
  Serial.println("Module A -> Master / Central");
  Serial.println("Module B -> Slave / Peripheral");

  String responseA =
    executeAndDisplay(moduleA, "AT+ROLE1");

  delay(1000);

  String responseB =
    executeAndDisplay(moduleB, "AT+ROLE0");

  delay(1000);

  String immeA =
    executeAndDisplay(moduleA, "AT+IMME0");

  String immeB =
    executeAndDisplay(moduleB, "AT+IMME0");

  (void)responseA;
  (void)responseB;
  (void)immeA;
  (void)immeB;
}

void dualUartMonitor()
{
  if (!moduleA.detected &&
      !moduleB.detected)
  {
    Serial.println("No detected modules.");
    return;
  }

  Serial.println();
  Serial.println("======================================");
  Serial.println(" Dual UART live monitor");
  Serial.println("======================================");
  Serial.println(
    "Incoming bytes from both modules are displayed."
  );
  Serial.println("Type /exit to return.");

  while (true)
  {
    while (moduleA.uart->available())
    {
      Serial.print("[A] ");
      Serial.write(moduleA.uart->read());
    }

    while (moduleB.uart->available())
    {
      Serial.print("[B] ");
      Serial.write(moduleB.uart->read());
    }

    String command = readUsbCommand();

    if (command.equalsIgnoreCase("/exit"))
    {
      return;
    }

    delay(1);
  }
}

// ==========================================================
// תפריט ראשי
// ==========================================================

void showMainMenu()
{
  Serial.println();
  Serial.println("======================================");
  Serial.println(" DUAL BLUETOOTH MODULE LABORATORY");
  Serial.println("======================================");
  Serial.println("SCAN");
  Serial.println("1 - Scan and identify Module A only");
  Serial.println("2 - Scan and identify Module B only");
  Serial.println("3 - Scan and identify both modules");
  Serial.println();
  Serial.println("STATUS");
  Serial.println("4 - Show Module A status");
  Serial.println("5 - Show Module B status");
  Serial.println("6 - Show both module statuses");
  Serial.println();
  Serial.println("CONFIGURATION AND TOOLS");
  Serial.println("A - Configure Module A");
  Serial.println("B - Configure Module B");
  Serial.println("M - Manual terminal for Module A");
  Serial.println("V - Manual terminal for Module B");
  Serial.println("P - Prepare A as BLE Master and B as BLE Slave");
  Serial.println("I - Run 30-second BLE inquiry on Module A");
  Serial.println("D - Dual UART live monitor");
  Serial.println("? - Show this menu");
  Serial.println();
  Serial.println("Serial Monitor: 115200 baud");
  Serial.println("Line ending: Any; recommended Both NL & CR");
  Serial.println();
  Serial.println("POWER: HC-05/HC-06 = 5V");
  Serial.println("POWER: HM-10 Clone/HMSoft BT4.0 = 3.3V from ESP32");
  Serial.println("Disconnect power before replacing a module.");
  Serial.println("======================================");
}

// ==========================================================
// setup ו-loop
// ==========================================================

void setup()
{
  Serial.begin(115200);
  delay(1200);

  Serial.println();
  Serial.println("======================================");
  Serial.println(" SERIAL MONITOR SETTINGS");
  Serial.println("======================================");
  Serial.println("Baud rate: 115200");
  Serial.println("Line ending: Any option is accepted");
  Serial.println("Recommended: Both NL & CR");
  Serial.println();
  Serial.println(
    "The program automatically sends the correct"
  );
  Serial.println(
    "line ending to each detected Bluetooth module."
  );
  Serial.println("======================================");
  Serial.println();
  Serial.println("======================================");
  Serial.println(" IMPORTANT MODULE POWER WARNING");
  Serial.println("======================================");
  Serial.println("HC-05 / HC-06: VCC = 5V");
  Serial.println("HM-10 Clone / HMSoft BT4.0:");
  Serial.println("VCC = 3.3V from the ESP32 3V3 pin");
  Serial.println();
  Serial.println("Disconnect power before changing module type.");
  Serial.println("Never power the BLE modules above from 5V.");
  Serial.println("Use a common GND between module and ESP32.");
  Serial.println("======================================");

  openModuleUart(moduleA, 9600);
  openModuleUart(moduleB, 9600);

  showMainMenu();
}

void loop()
{
  String choice = readUsbCommand();

  if (choice.length() == 0)
  {
    delay(1);
    return;
  }

  if (choice == "1")
  {
    scanAndIdentifyOne(moduleA);
  }
  else if (choice == "2")
  {
    scanAndIdentifyOne(moduleB);
  }
  else if (choice == "3")
  {
    scanAndIdentifyBoth();
  }
  else if (choice == "4")
  {
    showModuleStatus(moduleA);
  }
  else if (choice == "5")
  {
    showModuleStatus(moduleB);
  }
  else if (choice == "6")
  {
    showBothStatus();
  }
  else if (choice.equalsIgnoreCase("A"))
  {
    configureModule(moduleA);
    showMainMenu();
  }
  else if (choice.equalsIgnoreCase("B"))
  {
    configureModule(moduleB);
    showMainMenu();
  }
  else if (choice.equalsIgnoreCase("M"))
  {
    manualTerminal(moduleA);
    showMainMenu();
  }
  else if (choice.equalsIgnoreCase("V"))
  {
    manualTerminal(moduleB);
    showMainMenu();
  }
  else if (choice.equalsIgnoreCase("P"))
  {
    prepareMasterSlavePair();
  }
  else if (choice.equalsIgnoreCase("I"))
  {
    if (moduleA.type == ModuleType::HM10_CLONE ||
        moduleA.type == ModuleType::HMSOFT_V006)
    {
      runLongInquiry(moduleA);
    }
    else
    {
      Serial.println(
        "Module A must be an HM-10-compatible BLE module."
      );
    }
  }
  else if (choice.equalsIgnoreCase("D"))
  {
    dualUartMonitor();
    showMainMenu();
  }
  else if (choice == "?")
  {
    showMainMenu();
  }
  else
  {
    Serial.println("Unknown main-menu selection.");
    showMainMenu();
  }

  delay(1);
}