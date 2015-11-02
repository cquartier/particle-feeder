int motor = D0;
int led = D7;
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
String jamDetectedStr;

#define MST -7

#define MOTOR_TIME 5000
#define JAM_DETECT_DELAY 250

#define FEEDING_1_START_HOUR 8
#define FEEDING_1_END_HOUR 10
#define FEEDING_2_START_HOUR 18
#define FEEDING_2_END_HOUR 21

bool isDST();
bool isFeedingTime();
int cloudFeed(String command);
int feedCat();

bool isDST() {
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

bool isFeedingTime() {
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

int cloudFeed(String command) {
  return feedCat();
}

int feedCat() {
  int ret = 1;
  int timeLeft = MOTOR_TIME;
  digitalWrite(led, HIGH);
  digitalWrite(motor, HIGH);

  while (timeLeft > 0) {
    delay(JAM_DETECT_DELAY);
    timeLeft = timeLeft - JAM_DETECT_DELAY;
    if (analogRead(motorVoltage) > 1000) {
      // we've had a motor jam. Send a panic notice, shut off motor.
      Particle.publish("motor_jam",Time.timeStr(),60,PRIVATE);
      jamDetectedStr = Time.timeStr();
      timeLeft = 0; // break out of loop
      ret = -1;
    }
  }

  digitalWrite(motor, LOW);
  digitalWrite(led, LOW);

  time_t now = Time.now();
  saved.lastFeeding = now;
  EEPROM.put(0, saved);
  lastFeedingTimeStr = Time.timeStr(now);
  return ret;
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

  Particle.variable("lastReset", lastResetTimeStr);
  Particle.variable("lastSync", lastSyncTimeStr);
  Particle.variable("lastFeeding", lastFeedingTimeStr);
  Particle.variable("lastCheck", lastCheckTimeStr);
  Particle.variable("jamDetected", jamDetectedStr);

  Particle.function("feed", cloudFeed);
}

void loop() {

  Serial.println("Checking feeding time...");
  lastCheckTimeStr = Time.timeStr();

  if (isFeedingTime()) {
    feedCat();
  }

  if (Time.day() != Time.day(lastSyncTime)) {
    Serial.println("Resyncing clock");
    Particle.syncTime();
    if (isDST()) {
      Time.zone(MST + 1);
    } else {
      Time.zone(MST);
    }
    lastSyncTime = Time.now();
    lastSyncTimeStr = Time.timeStr(lastSyncTime);
  }

  delay(30 * 60 * 1000); // wait 30 minutes
}
