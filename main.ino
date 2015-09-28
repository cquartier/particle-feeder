int motor = D0;
int led = D7;
int motorTime = 3000;
int motorVoltage = A0;

struct NVDataStore {
  time_t lastFeeding;
};

NVDataStore saved;
time_t lastSyncTime;

String lastFeedingTimeStr;
String lastResetTimeStr;
String lastSyncTimeStr;
String lastCheckTimeStr;

#define MST -7

#define FEEDING_1_START_HOUR 8
#define FEEDING_1_END_HOUR 12
#define FEEDING_2_START_HOUR 16
#define FEEDING_2_END_HOUR 22

static bool isDST() {
  time_t now = Time.now();
  int month = Time.month(now);
  int day = Time.day(now);
  int dow = Time.weekday(now);
  int previousSunday = day - dow;
  // January, february, and december are out.
  if (month < 3 || month > 11) {
      return false;
  }
  // April to October are in
  if (month > 3 && month < 11) {
      return true;
  }

  // In march, we are DST if our previous sunday was on or after the 8th.
  if (month == 3) {
      return previousSunday >= 8;
  }
  // In november we must be before the first sunday to be dst.
  // That means the previous sunday must be before the 1st.
  return previousSunday <= 0;
}

static bool isFeedingTime() {
  time_t now = Time.now();
  int nowHour = Time.hour(now);
  bool ret = false;

  Serial.print("Last Feeding: ");
  Serial.println(saved.lastFeeding);

  if (nowHour >= FEEDING_1_START_HOUR && nowHour <= FEEDING_1_END_HOUR) {
    if (saved.lastFeeding == -1) {
      ret = true;
    } else {
      int lastHour = Time.hour(saved.lastFeeding);
      if (lastHour < FEEDING_1_START_HOUR || lastHour > FEEDING_1_END_HOUR) {
        ret = true;
      }
    }
  } else if (nowHour >= FEEDING_2_START_HOUR && nowHour <= FEEDING_2_END_HOUR) {
    if (saved.lastFeeding == -1) {
      ret = true;
    } else {
      int lastHour = Time.hour(saved.lastFeeding);
      if (lastHour < FEEDING_2_START_HOUR || lastHour > FEEDING_2_END_HOUR) {
        ret = true;
      }
    }
  }

  return ret;
}

static void feedCat() {
  digitalWrite(led, HIGH);
  digitalWrite(motor, HIGH);
  delay(100);
  if (analogRead(motorVoltage) > 1000) {
    // we've had a motor jam. Send a panic notice, shut off motor.
    digitalWrite(motor, LOW);
    digitalWrite(led, LOW);
    Spark.publish("motor_jam",Time.timeStr(),60,PRIVATE);
  } else {
    delay(motorTime);
    digitalWrite(motor, LOW);
    digitalWrite(led, LOW);
    time_t now = Time.now();
    saved.lastFeeding = now;
    EEPROM.put(0, saved);
    Spark.publish("feeding",Time.timeStr(now),60,PRIVATE);
  }
}

void setup() {
  Serial.begin(9600);

  Serial.println("Reset");


  Time.zone(MST); // set this as default timezone
  if (isDST()) {
    Time.zone(MST + 1);
  }
  lastResetTimeStr = Time.timeStr();
  lastSyncTime = Time.now();
  lastSyncTimeStr = Time.timeStr(lastSyncTime);
  EEPROM.get(0, saved);

  lastFeedingTimeStr = Time.timeStr(saved.lastFeeding);
  lastCheckTimeStr = Time.timeStr();

  pinMode(motor, OUTPUT);
  pinMode(led, OUTPUT);
  pinMode(motorVoltage, INPUT);

  Spark.variable("lastReset", lastResetTimeStr, STRING);
  Spark.variable("lastSync", lastSyncTimeStr, STRING);
  Spark.variable("lastFeeding", lastFeedingTimeStr, STRING);
  Spark.variable("lastCheck", lastCheckTimeStr, STRING);
}

void loop() {

  Serial.println("Checking feeding time...");
  lastCheckTimeStr = Time.timeStr();

  if (isFeedingTime()) {
    feedCat();
  }

  if (Time.day() != Time.day(lastSyncTime)) {
    Serial.println("Resyncing clock");
    Spark.syncTime();
    lastSyncTime = Time.now();
    lastSyncTimeStr = Time.timeStr(lastSyncTime);
  }

  delay(30 * 60 * 1000); // wait 30 minutes
}
