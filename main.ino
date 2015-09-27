int motor = D0;
int led = D7;
int motorTime = 2000;

int lastFeeding = 0;

#define MST -7

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

void setup() {

  Time.zone(MST); // set this as default timezone
  if (isDST())
  {
      Time.zone(MST + 1);
  }

  pinMode(motor, OUTPUT);
  pinMode(led, OUTPUT);

  Serial.begin(9600);
}

void loop() {
  //digitalWrite(led, HIGH);
  //digitalWrite(motor, HIGH);
  //delay(motorTime);
  //digitalWrite(motor, LOW);
  //digitalWrite(led, LOW);

  Serial.println(Time.timeStr());
  delay(1000);

  //delay(10 * 1000);
}
