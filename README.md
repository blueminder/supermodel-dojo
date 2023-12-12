<p align="center"><img src="SMD.png" alt="Supermodel Dojo Logo" width="600"><br /><i>Logo Credit: <a href="https://www.deviantart.com/mkfan12">@Frosted_Moontips</a></i></p>
<h1 align="center">Supermodel Dojo</h1>

**Supermodel Dojo** is a fork of [**Supermodel**](https://github.com/trzy/Supermodel), a Sega Model 3 emulator, with a focus on netplay features, replay, and training options for competitive play.  We intend to keep **Supermodel Dojo** updated with the latest downstream changes made to the parent project.

In addition to the features described above, Supermodel Dojo also contains a frontend for the emulator to make launching your favorite games and adjusting settings easier than ever.

# Supported Games

Currently, **Supermodel Dojo** only supports games with fighting game controls, including:

* Virtua Fighter 3
* Virtua Fighter 3 Team Battle
* Fighting Vipers 2

Replay and netplay functionality is planned for all games with local multiplayer on the same physical cabinet. Games that rely on digital controls are first in line.

# Frontend

Open `dojo.exe` to start the frontend. You may select any game and a menu will po up with available options.

![GAtgHsxWUAAN2Q7](https://github.com/blueminder/supermodel-dojo/assets/504581/1cd71f6b-081e-4ef0-8739-e5a2e46257c8)

To adjust Supermodel settings, click on the "Settings" button to show and hide the window.

![GAtgZA_XYAAUIKQ](https://github.com/blueminder/supermodel-dojo/assets/504581/ec8159a8-66b8-4c45-be24-dea9e94f5569)

# Replays

Session replay recording is enabled by default. To disable, be sure to disable the `RecordSession` option in `Supermode.ini`, or in the frontend's Settings Menu.

To load a replay via the frontend, click on your game of choice and select "Open Replay File". To open via the command line, add the `-replay-file=` argument with the replay path included.

## Replay Takeover

You can watch your old replays and drop in at any time as either character to experiment with your strategy and execution.

When watching a replay, you can assume control over a player by pressing `Shift + 1` for player 1 o `Shift + 2` for Player 2. This will transfer you to Training Mode with the remainder of the replay playing out.

# Training Mode

Supermodel Dojo's Training Mode lets you record, playback, and loop movements on training dummies you can cycle through with a "Switch Player" button.

Switch Player: `Shift + F7`
Record Slot (1-3): `Shift + (F1-F3)`
Record Slot (1-3): `Shift + (F4-F6)`
Toggle Playback Loop: `Shift + F9`

# Netplay

## Starting Netplay Session
### Hosting
1. Select a game and click **Host Netplay Session**.
2. Confirm Port & Input Delay
3. Press **Start**

![image](https://github.com/blueminder/supermodel-dojo/assets/504581/8dc0f01f-cbe2-4368-8ce5-1a46e889e6ec)

## Joining
1. Select a game and click **Join Netplay Session**.
2. Enter Host IP & Port
3. Press **Start**

![image](https://github.com/blueminder/supermodel-dojo/assets/504581/9a72b75f-7586-4b6d-b900-52d5d1365f81)

## Netplay Session Notes
The following settings should be identical between host & guest player to prevent desyncs:
* PowerPC Frequency
* Refresh Rate
* Delay

A wired connection is preferred, and guarantees a smoother connection between you and your opponent.

# Command Line Examples
## Hosting Netplay Session
```supermodel.exe ROMs/vf3.zip -netplay -delay=1 -target-port=6000 -host```

## Joining Netplay Session
```supermodel.exe ROMs/vf3.zip -netplay -delay=1 -target-port=6000 -target-ip="127.0.0.1"```

## Manual Delay Calculation
To calculate delay, we would use the following formula:

`Ceiling( Ping / 2 * FrameDuration (16 ms) ) = Delay #`

For instance, if my opponent’s average ping is 42 ms, I would divide it by 32 ms (2 * 16 ms) and round it up to 2.

```
= Ceiling( 42 ms / 2*16 ms )
= Ceiling( 42 ms / 32 ms )
= Ceiling( 1.3125 )
= 2
```

If you or your opponent are on WiFi, or you have a fluctuating connection, be sure to bump the delay up a little bit to compensate for this.
