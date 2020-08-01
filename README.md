Wi-Fi Connected Round LED Clock
===============================

Introduction
------------
Leon van den Beukel created a Wi-Fi Connected Round LED Clock that I instantly wanted to 3D print and build. It's a great little project, easy to print, easy to assemble, and it just plain makes me happy.

__Mostly__

The problem is that the Arduino software that was provided has a few problems:

1. The code, as written, doesn't allow for my time zone. I live in the United States Mountain time zone, which is UTC -7 hours. The time zone variable is unsigned. I can get around this by setting the time zone to 17 instead of -7, and modulus arithmetic in calculating the hour makes it all work out.
2. The code contains a function to determine Summer Time in the EU. I live in the United States where Daylight Saving Time differs from EU Summer Time.
3. WiFi credentials are stored in the code. If I accidentally sent a bug fix pull request, I could expose my WiFi credentials. Not really a big deal, but I don't like it. I'm going to explore putting the credentials into a settings file that is ignored by Git.
4. After running for some time, the clock freezes. This is a **huge** deal. A clock that stops after any amount of elapsed time makes the clock useless.

My goal is to write updated software from scratch to fix these problems so that I can take advantage of this fantastic project to its fullest.

Original Project Link
---------------------
Leon van den Beukel's [github repository](https://github.com/leonvandenbeukel/Round-LED-Clock) This contains his .stl files for printing, his parts list, a simple text-based schema (though I want to create a graphic--stay tuned), and a link to his YouTube video.

Time Library Used
-----------------
Date and time was made easy by using Paul Stoffregen's Time library. It's found [here](https://github.com/PaulStoffregen/Time). It is simple, elegant, and intuitive. The only difficult part was connecting to WiFi and getting the time from NTP. And it was only difficult until I looked at Paul's sample code that made the job super easy. No need any more to deal with things like leap years and lengths of months in doing the time conversion. 

Daylight Saving Time
--------------------
Since I'm in the United States, I started out with Daylight Saving Time. However, since the original author is in the European Union and uses Summer Time, I decided to use his code for Summer Time and add my code for Daylight Saving Time, and allow the user to specify which is to be used in the settings.h file. One of two choices are available:
* #define USE_DAYLIGHT_SAVING_TIME true
* #define USE_SUMMER_TIME true

If neither option is defined, the clock will stay on standard time year round.
