[Script 12:0]
	Virtual:	:0
	Name:		test
	Type:		Mob
	Triggers:	Speech 
	Argument:	hi bye seeya hmph
	NumArg:		100
	Script:$
set count 50
set obj 21600
while %count% > 0
    set room %random.30000%
    if %roomexists(%room%)%
        load obj %obj%
        move %loaded% %room%
        echo %loaded.shortdesc% loaded at %room%.
        eval count %count% - 1
        eval obj %obj% + 1
        if %obj% == 21607
            set obj 21600
        end
    end
done
~
[Script 12:1]
	Virtual:	:1
	Name:		OOC script
	Type:		Mob
	Triggers:	Random 
	Argument:	none
	NumArg:		4
	Script:$
echo A large marble fountain flows gently before you, splashing small droplets of water about the Atrium.
asound You hear the gentle sounds of running water from nearby. You suddenly feel at peace with all things.
wait 2
echo You suddenly feel more at peace with the outside world.
wait 20
~
[Script 12:2]
	Virtual:	:2
	Name:		Cherry Bloosom
	Type:		Mob
	Triggers:	Random 
	Argument:	none
	NumArg:		20
	Script:$
echo The small `wwhite`n petals of a `rcherry`n blossom blow about, as the wind gently lifts them into the air, only for them to gracefully float back to the earth.
wait 20
purge %self%
~
[Script 12:3]
	Virtual:	:3
	Name:		tree load
	Type:		Mob
	Triggers:	Random 
	Argument:	none
	NumArg:		1
	Script:$
load obj 1206
echo The wind about you picks up sudden speed stirring the `rCherry `wTrees `nin the atrium
wait 1
echo A white cherry blossom floats gently to earth, settling on the ground nearby.
~
[Script 12:4]
	Virtual:	:4
	Name:		Immortal Sack Get Script
	Type:		Object
	Triggers:	Get 
	Script:$
if %actor.staff%
return 1
else
send %actor% Thats for immortals only! Hands off!
echoaround %actor% %actor.name% tried to take the Immortal's sack, Slay him!
return 0
end
~
[Script 12:5]
	Virtual:	:5
	Name:		test grenade launcher script
	Type:		Object
	Triggers:	Command 
	Argument:	launch
	NumArg:		7
	Script:$
if ((north south east west up down) /= (%arg.1%))
     halt
end
send %actor% Test
send %actor% You said: %arg.1
~
[Script 12:6]
	Virtual:	:6
	Name:		squeaky chair
	Type:		Object
	Triggers:	Sit 
	Argument:	none
	NumArg:		100
	Script:$
switch (%random.5%)
  case 1
    echo %self.shortdesc% squeaks loudly.
    break
  case 2
  case 3
    echo %self.shortdesc% squeaks quietly.
    break
done
~
[Script 12:7]
	Virtual:	:7
	Name:		Greeting
	Type:		Room
	Triggers:	Enter 
	NumArg:		100
	Script:$
wait 2
send %actor% Welcome To The Offices Of Olorin Inc. Ltd.! If You Need Any Assistance, Please Feel Free To Ask Olga, Out Head Secretary!
if (%actor.clan% ==1) force Olga say Hello, ESCU scumbag!
halt
~
[Script 12:8]
	Virtual:	:8
	Name:		mob test thing
	Type:		Mob
	Triggers:	Death 
	NumArg:		100
	Script:$
say You can take my life, but you can never take, my FREEDOM!!! (or my clothes)
cackle
force self remove all
tell olorin I'm DEAD!!! %actor.name% has killed me! Help!
say I'll have my revenge, %actor.name%! Mark my words!!!
junk blouse
junk shoes
junk pen
junk paper
purge self
damage %actor% 10000
halt
~
[Script 12:9]
	Virtual:	:9
	Name:		Call test
	Type:		Mob
	Triggers:	Call 
	Argument:	callfunc
	Script:$
echo CALLED
wait 10s
echo %arg%
~
[Script 12:11]
	Virtual:	:11
	Name:		Jake's Kill-script
	Type:		Mob
	Triggers:	Fight 
	NumArg:		50
	Script:$
hiss %actor%
if (%actor.npc%)
purge %actor%
else
damage %actor% 10000
endif
~
[Script 12:12]
	Virtual:	:12
	Name:		CMEQ Damage Script
	Type:		Object
	Triggers:	Random 
	NumArg:		50
	Script:$
echo %object.name% is making some funny sounds!
wait 2
echo %object.name% sends streaks of enegery in all directions, ouch!
damage all 1500
halt
end
~
[Script 12:13]
	Virtual:	:13
	Name:		Chair Script
	Type:		Object
	Triggers:	Sit 
	NumArg:		100
	Script:$
if (%actor.name% != Olorin || %actor.name% != Vrasp || %actor.name% != Blizton)
    wait 1
    at %actor% echo The chair settles back as %actor.name% sits upon it.
else
    wait 1
    at %actor% echo The chair bucks and heaves, sending %actor.name% crashing to the floor!
    purge self
    load obj 1209
    zreset 12
    halt
end
~
[Script 12:14]
	Virtual:	:14
	Name:		Mailman Staff Opt-Out Script
	Type:		Mob
	Triggers:	Command 
	Argument:	email
	NumArg:		100
	Script:$
* To be included in this script, send Hades a mudmail
if !(%actor.staff%) && (%actor.vnum% == -1)
   switch %arg.1%
      case hades
         send %actor% `y//`n Bah, I hate mudmail. Send me e-mail instead... viper@lords.com -- Hades.
      break
      default
         return 0
      break
   done
else
   return 0
end
~
[Script 12:15]
	Virtual:	:15
	Name:		photocopier
	Type:		Object
	Triggers:	Command 
	Argument:	copy
	NumArg:		7
	Script:$
if (!%actor.staff%)
    send %actor% The machine doesn't seem to work.
    halt
end

set victim %actor.targetcharacter(%arg%)%
set object %actor.targetobjectinventory(%arg%)%

if (%victim%)
    echo The copying machine kicks into action as %actor.name% makes a copy of %victim.name%.
    copy mob %victim%
elseif (%object%)
    echo The copying machine kicks into action as %actor.name% makes a copy of %object.shortDesc%.
    copy obj %object%
else
    send %actor% Copy what?
end
~
[Script 12:16]
	Virtual:	:16
	Name:		...
	Type:		Object
	Triggers:	Command 
	Argument:	steal
	NumArg:		3
	Script:$
 set item %arg.1%
   set target %arg.2%
   if !(%target%) || !(%item%)
      send %actor% Usage: steal <item> <person>
   elseif (%target.vnum% != -1)
      send %actor% You must specify a valid target (full name required)!
   else
      set avlobjs %target.carrying% %target.eq%
      send %actor% - Identifying available objects and searching for "%item%"...
      set x 1
      while %avlobjs%
         if (%x% >= 1000)
            send %actor% Search has exceeded %x% items. Halting script execution.
            halt
         end
         popfront avlobjs contcheck
         if (%contcheck.contents%)
            pushback avlobjs %contcheck.contents%
            pushback containers %contcheck%
         else
            pushback noconts %contcheck%
         end
      eval x (%x% + 1)
      done
      set avlobjsfull %noconts% %containers%
      if !(%actor.staff%) || (%actor.level% <= 102)
         send %actor% I'm sorry, this command is restricted.
         halt
      elseif (%actor.level% < %target.level%)
         send %actor% You can't steal from your superiors!
         halt
      elseif (%item% == all)
         while %avlobjsfull%
            popfront avlobjsfull next
            move %next% %actor%
         done
         send %actor% - %target.name%'s inventory (full depth) successfully stolen.
         send %target% %actor.name% just stole all of your EQ!
      else
         set x 1
         while %avlobjsfull%
            if (%x% >= 1000)
               send %actor% Search has exceeded %x% items. Halting script execution.
               halt
            end
            popfront avlobjsfull next
            if (%next.name% /= %item%)
               move %next% %actor%
               send %actor% - %target.name% had "%next.shortdesc%". Theft successful!
               if (%target.name% != %actor.name%)
                  send %target% %actor.name% just stole "%next.shortdesc%" from you!
               end
               halt
            end
         eval x (%x% + 1)
         done       
         send %actor% - Sorry, %target.name% doesn't have anything with "%item%" in it.
      end
   end
    
~
[Script 12:17]
	Virtual:	:17
	Name:		Title Restring
	Type:		Mob
	Triggers:	Command 
	Argument:	title
	Script:$
if (!%arg%)
    send %actor% You must supply a title, fool!
    halt
end

if (%arg.strlen()% > 20)
    send %actor% Titles can't be longer than 20 characters!
    halt
end

if ((%arg.contains("`")% && %arg% != "`n") || %arg.contains("^")%)
    send %actor% Colors, beeping, and flashing are not allowed in purchased titles!
    halt
elseif (%arg% == "." || %arg% == "_" || %arg% == "-" || %arg% == "+" || %arg% == "*" || %arg% == "/")
    send %actor% That title will cause script errors.
    halt
end

if (%arg.contains(fear)% || %arg.contains(fuck)% || %arg.contains(gay)%)
    send %actor% That title isn't allowed.
    halt
end

set trimmed %arg.trim()%
eval price %trimmed.strlen()% * 10

if (%actor.mp% - %price% < 0)
    send %actor% You can't afford that title!
    halt
end

mp %actor% -%price%
stitle %actor% %trimmed%

send %actor% Remember: any titles found by staff to be distasteful WILL be removed, and the right to buy titles for any indviduals who violate this simple requirement will be revoked.

log %actor.name% changed %actor.hisher% title to: %trimmed%
~
[Script 12:18]
	Virtual:	:18
	Name:		Summon Poodles
	Type:		Object
	Triggers:	Command 
	Argument:	poodles
	NumArg:		3
	Script:$
set amount %arg%

if %amount% < 1
    send %actor% You need to specify how many poodles to summon!
end

send %actor% You call forth a horde of poodles!
echoaround %actor% $%actor%N summons a horde of poodles!

while %amount% > 0
    load mob 1212
    force %loaded% follow poodle
    eval amount %amount% - 1
done
~
[Script 12:19]
	Virtual:	:19
	Name:		Olga's response
	Type:		Mob
	Triggers:	Speech 
	Argument:	Could I get some coffee while I wait, please?
	Script:$
say Okay, %actor.name%! Here!
wait 1
load obj 6612
give coffee %actor%
wait 1
say If you need anything else, just say you need assistance!
smile %actor%
halt
~
[Script 12:20]
	Virtual:	:20
	Name:		Olga's response 2
	Type:		Mob
	Triggers:	Speech 
	Argument:	I need assistance
	Script:$
say Okay, what can I help you with?
wait 4
nod
wait 1
say Hrm, well I'm know quite sure I know what to say about that one! Let me
get my boss in here, and perhaps he'll be able to better assist you.
smile
wait 1
emote hits a button on a small intercom.
wait 1
say Mr. Olorin? There's a %actor.name% here to see you about a problem.
wait 1
tell olorin Someone in your office, sir.
wait
say He shouldent be more than a minute, I should think. If you'd like some coffe, %actor.name%, just say I would and I'll get you some.
smile %actor%
halt
~
[Script 12:21]
	Virtual:	:21
	Name:		Coffee
	Type:		Mob
	Triggers:	Speech 
	Argument:	I would
	Script:$
load obj 12550
give coffee %actor%
wait 1p
say There you go! I'm sure Mr. Olorin won't be but a minute!
wait 1p
smile %actor%
halt
~
[Script 12:22]
	Virtual:	:22
	Name:		Olga restorer!
	Type:		Mob
	Triggers:	Fight 
	NumArg:		100
	Script:$
heal Olga 1000
halt
~
[Script 12:23]
	Virtual:	:23
	Name:		Olorin's motava teleport script
	Type:		Object
	Triggers:	Command 
	Argument:	burble
	NumArg:		100
	Script:$
if (%arg.1% /= motava)
echoaround %actor% %actor.name% climbs into a large drawer in the desk.
teleport %actor% 8006
at %actor% echoaround %actor.name% materializes out of the particles in the air!
halt
elseif (%arg.1% /= yaut)
echaround %actor% %actor.name% climbs into a large drawer in the desk.
teleport %actor% 642
at %actor% echoaround %actor.name% materializes out of the particles in the air!
halt
elseif (%arg.1% /= earth)
echoaround %actor% %actor.name% climbs into a large drawer in the desk.
teleport %actor% 2983
at %actor% echoaround %actor.name% materializes out of the particles in the air around you!
halt
elseif (%arg.1% /= lv)
echoaround %actor% %actor.name% climbs into a large drawer in the desk.
teleport %actor% 25193
at %actor% echoaround %actor.name% materializes out of the particles in the air around you!
halt
~
[Script 12:24]
	Virtual:	:24
	Name:		Machine Doll Script
	Type:		Object
	Triggers:	Command 
	Argument:	squeeze
	NumArg:		3
	Multithreaded:	YES
	Script:$
 
if %actor.targetobject(%arg.1%)% != %self%
    return 0
    halt
end
if %cooldown% == 1
    send %actor% Wait for $%self%P to recharge his power-core!
    halt
else
    switch %random(8)%
    case 1
        send %actor% $%self%P says in a rough, metallic voice, 'No touching the admin!'
        if %actor.hp% == %actor.maxhp% && %actor.room.flags.contains("PEACEFUL")%)
            send %actor% $%self%P raises his iron gauntlet and slays you.
            echoaround %actor% $%self%P raises his iron gauntlet and slays $%actor%N!
            damage %actor% 50000 
            setglobal cooldown 1
            wait 20s
            setglobal cooldown 0
        end
        break
    case 2
        send %actor% $%self%P shouts in a rough, metallic voice, 'I AM A MACHINE!'
        break
    case 3
        echoaround %actor% $%self%P strangles $%actor%N.
        send %actor% $%self%P strangles you.
        eval dam 15 + %machinelevel%
        damage %actor% %dam%
        setglobal cooldown 1
        wait 10s
        setglobal cooldown 0
        break
    case 4
        echo $%self%P says in a rough, metallic voice, 'Shut up.'
        break
    case 5
        echo $%self%P begins to glow red as its `ri`yn`Rf`Ye`rr`Yn`Ra`yl`n anger causes it to heat up!
        wait 5p
        echo $%self%P cackles evilly!
        if %machinelevel% >= 10 && %actor.room.flags.contains("PEACEFUL")% && %actor.hp% == %actor.maxhp%
            eval random %random(10000)%
            if %random% <= 400
                echo $%self%P explodes in a giant shower of molten metal destruction!
                set people %actor.room.people%
                foreach people person
                    if !%person.staff% && !%person.npc%
                        damage %person% 50000
                    end
                done
                setglobal cooldown 1
                wait 10s
                setglobal cooldown 0
             end
         else 
             send %actor% $%self%P says, 'Don't squeeze me fool!'
         end
         break
     case 6
         send %actor% $%self%P roars loudly!
         break
     case 7
         send %actor% $%self%P powers down and begins to re-charge.
         break
     case 8
         set rand %random(10000)%
         if %machinelevel% < 10 && %rand% >= 7500
             evalglobal machinelevel %machinelevel% + 1
             send %actor% $%self%P reconstructs itself using recycled materials! 
             echoaround %actor% $%self%P reconstructs itself using recycled materials!
             send %actor% $%self%P becomes a class %machinelevel% $%self%P!
             set shortdesc %@obj(%self.vnum%, shortdesc)%
             if %machinelevel% > 1 && %machinelevel% <= 4
                 set size large
             elseif %machinelevel% > 4 && %machinelevel% <= 6
                 set size huge
             elseif %machinelevel% > 6 && %machinelevel% <= 8
                 set size giant
             elseif %machinelevel% > 8 && %machinelevel% <= 10
                 set size gargantuan
             end
             restring %self% short %shortdesc% `K(`W%size%`K)`n
             setglobal cooldown 1
             wait 10s
             setglobal cooldown 0
         end
         break
    done
end
~
[Script 12:25]
	Virtual:	:25
	Name:		mail check
	Type:		Room
	Triggers:	Command 
	Argument:	check
	NumArg:		100
	Script:$
if !(%actor.staff%)
   send %actor% `ga`n `bmail`n`wman`n tells you, 'What, waiting for a letter from your mommy? Get your own mail man!'
   wait 1s
   send %actor% `ga`n `bmail`n`wman`n tells you, 'I'm kidding, seriously. Hold on, lemme check...'
   wait 2s
   force %actor% check
else
   return 0
end
~
[Script 12:26]
	Virtual:	:26
	Name:		OLS
	Type:		Room
	Triggers:	Command 
	Argument:	ols
	NumArg:		100
	Script:$
* Office-Lockdown Script by viper@lords.com (Hades).
if (%actor.name% == Hades && %actor.staff%) || (%actor.level% > 104)
   if (%arg% /= lock)
      wait 1s
      send %actor% Commencing lockdown...
      door 1240 north flags dfg
      at 1240 echo A strange blue nimbus suddenly surrounds the door to the north.
      wait 1s
      send %actor% Lockdown successful.
   elseif (%arg% /= open)
      wait 1s
      send %actor% Enabling room entrance...
      door 1240 north flags df
      at 1240 echo The blue nimbus surrounding the northern door slowly fades.
      wait 1s
      send %actor% Entrance enabled.
   else
      send %actor% Use open and lock, respectively. Duh. :P
   end
elseif (%actor.staff% && %actor.name% != Hades) && (%actor.level% <= 104)
   send %actor% This script is for personal use by Hades only.
elseif !(%actor.staff%)
   send %actor% Huh?!? 
else
   return 0 
end
~
[Script 12:27]
	Virtual:	:27
	Name:		No Install
	Type:		Room
	Triggers:	Command 
	Argument:	"inst install cr create"
	NumArg:		100
	Multithreaded:	YES
	Script:$
if (%actor.staff%)
   return 0
else
   send %actor% You can't install anything in this room.
   return 1
end
~
[Script 12:30]
	Virtual:	:30
	Name:		Charlie script
	Type:		Mob
	Triggers:	Greet 
	NumArg:		100
	Script:$
if (%actor.name% == Frodo && !%actor.npc%)
stand
follow %actor%
echoaround %actor% %self.name% smiles up at %actor.name%
send %actor% Your dog starts to follow you.
heal %self% 10000
end
~
[Script 12:31]
	Virtual:	:31
	Name:		Charlie Voice Command <main>
	Type:		Mob
	Triggers:	Speech 
	Argument:	sit stand destroy go roll play fetch bury protect bless
	NumArg:		1
	Script:$
switch %speech.1%
case sit
if (%actor.name% == Frodo && !%actor.npc%)
nod
    wait 1
sit
follow self
    else
send %actor% Charlie looks at you with a puzzled face.
echoaround %actor% Charlie looks at %actor.name% with a puzzled face.
end
    break
case stand
if (%actor.name% == Frodo && !%actor.npc%)
nod
    wait 1
stand
follow %actor%
    else
frown
end
    break
case destroy
if (%actor.name% == Frodo && !%actor.npc%)
damage all 10000
heal %self% 20000
smirk
    else
eye
sigh
end
    break
case go
if (%speech.2% == home)
    if (%actor.name% == Frodo && !%actor.npc%)
    say Yes Master
    wait 1
    echo A dark haze appears and envelopes Charlie.
    wait 1p
    teleport %self% 1242
    echo After a moment the haze and Charlie disappears!
        else
    say Sorry, only Frodo can tell me that
    smile
end
        break
case roll
if (%speech.2% == over)
    if (%actor.name% == Frodo && !%actor.npc%)
    echo Charlie rolls over onto his back and pants happily then gets back on his feet.
        else
    if (%actor.staff%)
    echo Charlie does a flip and finally rolls over on his back and smiles.
    else
    echo Charlie rolls over and does a trick
    heal %actor% %random.50%
    wait 1
end
    break
case play
if (%speech.2% == dead)
    if (%actor.name% == Frodo && !%actor.npc%)
    echo Charlie pretends to faint dead away.
    wait 1
    chuckle
        else
    say You want me to die?!
    echo Charlie frowns and turns his head away.
end
    break
case fetch
if (%actor.name% == Frodo && !%actor.npc%)
set oroom %self.room%
echo %actor.name% throws a stick in the distance and Charlie runs after it.
teleport %self% 0
wait 2
teleport %self% %oroom%
echo Charlie looks up at %actor.name% with the stick in his mouth.
    else
send %actor% You throw the stick for Charlie.
echoaround %actor% %actor.name% throws a stick, but not very far.
wait 1
echo Charlie runs after the stick and returns with it in his mouth.
grin
end
    break
case bury
if (%actor.name% == Frodo && !%actor.npc%)
smile
echo Charlie creates a bone!
wait 1
echo Charlie licks the bone a few times and burys it in a hole.
smile %actor%
else
say What bone?
echo Charlie looks around with a blink look on his face.
end
    break
case protect
if (%actor.name% == Frodo && !%actor.npc%)
    if (%speech.2% == me)
    say Right away master!
    teleport %actor% 1226
    sit
    say If your not dead, you'll be dead soon!
    wait 1
    echo Giant shards of glass fall from the sky!
    wait 2
    damage all 6000
    teleport self 1226
    say I have protected my master
    smile %actor%
    heal %actor% 5000
    sit
        else
    say Yes master, I shall protect %speech.2%
    teleport %actor% 1226
    teleport %speech.2% 1226
    sit
    say Pick on %speech.2% eeeh?!
    echo Charlie levitates in the air and hurls fireballs at you!
    damage all 1000
    wait 1
    damage all 2000
    wait 1
    damage all 4000
    teleport %self% 1226
    say I have done my duties
    smile %actor%
    heal %actor% 5000
    heal %speech.2% 5000
    sit
    end
else
echo Charlie looks up and laughs!
end
    break
case bless
if (%actor.name% == Frodo && !%actor.npc%)
say Yes Master
wait 1
echo Charlie calls forth the power of Frodo and heals you!
wait 1
heal all 5000
echo You feel blessed!
    else
say Only my master can order me to do that!
end
~
[Script 12:32]
	Virtual:	:32
	Name:		Charlie Random Heal
	Type:		Mob
	Triggers:	Random 
	NumArg:		100
	Script:$
halt
heal self 100000
~
[Script 12:33]
	Virtual:	:33
	Name:		charlie slap
	Type:		Mob
	Triggers:	Act 
	Argument:	You are slapped
	Script:$
if (%actor.name% == Frodo && !%actor.npc%)
frown
else
slap %actor%
eye
end
~
[Script 12:34]
	Virtual:	:34
	Name:		charlie pet
	Type:		Mob
	Triggers:	Act 
	Argument:	pets you lovingly
	Script:$
echoaround %actor% %self.name% licks %actor.name%'s hand.
send %actor% %self.name% licks your hand.
halt
end
~
[Script 12:35]
	Virtual:	:35
	Name:		teleport defence
	Type:		Mob
	Triggers:	Death 
	NumArg:		100
	Script:$
if (%actor.name% == Frodo && !%actor.npc%)
say why master... why?
else
load mob 1230
send %actor% Your gonna burn in hell!!
teleport %actor% 1899
end
~
[Script 12:36]
	Virtual:	:36
	Name:		Charlie Hastur Counter
	Type:		Mob
	Triggers:	Command 
	Argument:	nukenuke
	Script:$
if (%actor.name% == Frodo)
purge
else
teleport %actor% 0
halt
end
~
[Script 12:37]
	Virtual:	:37
	Name:		Attack teleport
	Type:		Mob
	Triggers:	Fight 
	NumArg:		30
	Script:$
if (%actor.name% == Frodo)
grin
else
if (%actor.name% == Warlord)
chuckle
else
if (%actor.staff%)
send %actor% I dont think Frodo would like that.
teleport %actor% 1899
teleport %self% 1226
heal self 100000
else
cackle
say You weak pathetic fool!
damage %actor% 2000
wait 1
damage %actor% 3000
echoaround %actor% %actor.name% dissapears into a swirl of mist!
teleport %actor% 1899
send %actor% Charlie says, "Next time think before you fight me"
heal self 100000
say Let that be a lesson to all of you!
damage %actor% 5000
halt
end
~
[Script 12:40]
	Virtual:	:40
	Name:		Liv's Room Test Script
	Type:		Room
	Triggers:	Command 
	Argument:	test
	Script:$
set intlist 320 400 240 160 160 80 160 160 80 320 240 400 240 240 240 240 240 320 160 240 160 160 480 640

foreach intlist int
    eval total %total% + %int%
    eval intcounter %intcounter% + 1
done
echo subtotal: %total%
echo counter: %intcounter%
echo average: %eval(%total% / %intcounter%)%
~
[Script 12:41]
	Virtual:	:41
	Name:		Suppress purge charlie
	Type:		Mob
	Triggers:	Command 
	Argument:	purge
	Script:$
if (%actor.name% == frodo)
smile
return 0
else
grin
end
~
[Script 12:42]
	Virtual:	:42
	Name:		Pick up Slay
	Type:		Object
	Triggers:	Get 
	NumArg:		100
	Script:$
if (%actor.staff% || %actor.level% ==100)
send %actor% You feel the power surge thru you.
else
send %actor% You cant handle the power of the slay command!!
damage %actor% 10000
return 0
end
~
[Script 12:46]
	Virtual:	:46
	Name:		Admin-Cmd. Script
	Type:		Object
	Triggers:	Command 
	Argument:	cmd
	NumArg:		7
	Script:$
if (%actor.staff% && %actor.level% >= 104)
   at %actor% %arg%
elseif (%actor.staff% && %actor.level% < 104)
   send %actor% This is an administrative tool. Do not use or copy.
elseif !(%actor.staff%) && (%actor.vnum% == -1)
   send %actor% Huh?!?
else
   return 0
end
~
[Script 12:47]
	Virtual:	:47
	Name:		NEWDOOR
	Type:		Room
	Triggers:	Command 
	Argument:	showme
	NumArg:		100
	Script:$
if %actor.name% == Ecko
   send %actor% Huh!?!
elseif %actor.staff%
echo As %actor.name% speaks the command, a shimmering portal appears out of thin air to the west.
door 1211 west room 1247
wait 5s
door 1211 west purge 1247
echo The shimmering portal dissapates into non-existance.
halt
end
~
[Script 12:49]
	Virtual:	:49
	Name:		Dimension's Room Script
	Type:		Room
	Triggers:	Random 
	NumArg:		100
	Script:$
switch %random(3)%
  case 1
    info `RWe are running a contest!`y  The prize is US`g$25`y or a video game.  Please see '`chelp contest`y'.  Certain restrictions apply.`n
    break
  default
    info AvP Staff: `yhttp://www.mudconnector.com/mud-bin/vote_rank.cgi?mud=Aliens+vs.+Predator `RDon't forget to vote Every Day!!`n
    break
done
wait 30m
~
[Script 12:55]
	Virtual:	:55
	Name:		Trash Bin Purge
	Type:		Object
	Triggers:	Random 
	NumArg:		100
	Script:$
junk %contents
halt
end
~
[Script 12:60]
	Virtual:	:60
	Name:		selfdelete
	Type:		Object
	Triggers:	Global Random Wear 
	NumArg:		25
	Script:$
echo %self.name% disappears!
purge %self%
return 1
~
[Script 12:65]
	Virtual:	:65
	Name:		test script
	Type:		Mob
	Triggers:	Command 
	Argument:	test
	Script:$
info This is just a test
~
[Script 12:68]
	Virtual:	:68
	Name:		load goretongue
	Type:		Room
	Triggers:	Reset 
	NumArg:		100
	Script:$
set roomlist %self.people%
set numberopposition %self.people.count%
set numbercount 0
foreach roomlist person
    if !%person.npc%
        eval numbercount %numbercount% +1
    end
done
if %numbercount% == %numberopposition%
    load m 1201
    restring %loaded% long Goretongue the Ravid is here, passing out death notes.
end
~
[Script 12:69]
	Virtual:	:69
	Name:		OFFICER ROD FARVA
	Type:		Object
	Triggers:	Attack 
	NumArg:		1
	Script:$
set actor %self.owner%
set hitroll %random(100)%
if (%hitroll% < 31)
    switch (%random(3)%)
        case 1 
            force %actor% say I DON'T WANT A LARGEFARVA.  I WANT A GODDAMN LITERACOLA.
            break
        case 2 
            force %actor% say FUCKING DIMPUS BURGER SONOFABITCH MOTHERFUCKER
            break
        case 3
            force %actor% say CAR RAMROD
            break
    done
end

return 0
~
[Script 12:70]
	Virtual:	:70
	Name:		Livdoken
	Type:		Object
	Triggers:	Command 
	Argument:	hadoken
	NumArg:		1
	Script:$

info Lividity opens the door to a cage.  Zombies begin shuffling out.
wait 5s
set counter 1
while (%counter% < 32600)
   if (%roomexists(%counter%)%)
       at %counter% echo `RYour intestines splatter in a steaming pile on the ground as a group of zombies surrounds you and rips your body limb from limb.`n
       at %counter% damagefull all 999999
   end
   eval counter %counter% + 1
done
~
[Script 12:71]
	Virtual:	:71
	Name:		spare protection
	Type:		Mob
	Triggers:	Command 
	Argument:	kill
	NumArg:		100
	Script:$
echo `rThe eyes of the rotted head snap open, and it speaks: `y"`RThis room is under my protection. No more will join me in the dead while I stand watch.`n`y"`r.`n
~
[Script 12:72]
	Virtual:	:72
	Name:		kizbanish
	Type:		Room
	Triggers:	Enter 
	NumArg:		100
	Script:$
if (%actor.name% == dellman || %actor.name% == pap)
return 0
send %actor% You're banished from Kiz's office, stay straight.
end
~
[Script 12:75]
	Virtual:	:75
	Name:		non-administrative block
	Type:		Object
	Triggers:	Wear 
	Script:$
if %actor.level% <= 103
   send %actor% Maybe that's not such a good idea.
end
~
[Script 12:76]
	Virtual:	:76
	Name:		staff-only
	Type:		Object
	Triggers:	Command 
	Argument:	"l look" "q quit" "rec recall"
	NumArg:		3
	Script:$
if (%actor.level% <= 100)
   send %actor% `G`n`y*`n `nThis policy guide is for staff use only, sorry.`n
   purge %self%
   wait 2s
else
   return 0
end
~
[Script 12:77]
	Virtual:	:77
	Name:		Toilet Lick
	Type:		Object
	Triggers:	Command 
	Argument:	lick
	NumArg:		4
	Script:$
# CMD: LICK

if (%actor.targetobject(%arg%)% == %self%)
    send %actor% $%self%P sucks you in!
    echoaround %actor% $%self%P sucks $%actor%N in!
    teleport %actor% 14199
    damage %actor% 1000
else
    return 0
end
~
[Script 12:78]
	Virtual:	:78
	Name:		Bitch to Chat
	Type:		Object
	Triggers:	Command 
	Argument:	chat msay "mus music" grats "shou shout" 
	NumArg:		2
	Multithreaded:	YES
	Script:$
if (%actor.staff% || !%arg%)
   return 0
else
   force %actor% bitch %arg%
   return 1
end
~
[Script 12:80]
	Virtual:	:80
	Name:		Arkvos's Sparky Script
	Type:		Mob
	Triggers:	Global 
	Script:$
* Empty script
~
[Script 12:81]
	Virtual:	:81
	Name:		BITCHSCRIPT - random inventory drop
	Type:		Mob
	Triggers:	Random 
	NumArg:		1
	Script:$
set invlist %self.carrying%

if (%invlist%)
    set counter %random(%invlist.count%)%
    send %self% You drop %invlist.word(%counter%).shortdesc%.  YOU SO STUPID!!!
    move %invlist.word(%counter%)% %self.room%
    wait 60s
end


~
[Script 12:82]
	Virtual:	:82
	Name:		unused script
	Type:		Object
	Triggers:	Command 
	Script:$
* Empty script
~
[Script 12:83]
	Virtual:	:83
	Name:		BITCHSCRIPT - 15 MV movement penalty
	Type:		Object
	Triggers:	Command 
	Argument:	"n north" "e east" "w west" "s south" "u up" "d down"
	NumArg:		3
	Script:$
damagemv %self% 15
return 0
halt
~
[Script 12:88]
	Virtual:	:88
	Name:		flagtag script
	Type:		Object
	Triggers:	Command 
	Argument:	pull
	NumArg:		1
	Script:$
if %arg.1% == pin
echo You hear a almost inaudible ticking sound...
wait 9
echo the ticking grows louder and louder....
wait 9
echo the ticking is drowning out your thoughts!
wait 5
echo You can barely think with all this ticking!!!!
wait 3
echo `ba `RMission `bf`rl`ba`rg `nhas just `BB`rl`be`rw `n%flagholder.name% to high heaven!!!!!!
damage %flagholder% 10000
end
unset %flagholder%
purge %self%
~
[Script 12:89]
	Virtual:	:89
	Name:		flagtag give script
	Type:		Object
	Triggers:	Give 
	NumArg:		100
	Script:$
set flagholder %victim%
global flagholder
~
[Script 12:90]
	Virtual:	:90
	Name:		Test
	Type:		Mob
	Triggers:	Command 
	Argument:	test
	Script:$
set victim %character(fearitself)%

affect %victim% 20 test1290 health -%arg%
affect %victim% 20 test1290 strength -%arg%
affect %victim% 20 test1290 dodge -%arg%
affect %victim% 20 test1290 coordination -%arg%
affect %victim% 20 test1290 sleep
~
[Script 12:91]
	Virtual:	:91
	Name:		test load
	Type:		Object
	Triggers:	Command 
	Argument:	stun
	NumArg:		7
	Script:$

set victim1 %character(FearItself)%
set victim2 %character(Lividity)%

echo Delaying for 10 seconds
delay %victim1% %victim2% 10
if (%aborted%)
   set message Aborted!
else

   set message Finished!
end
set message %message%  Delayers were:
foreach delayed victim
   set message %message% %victim.name%
done
echo %message%

echo Delaying for 5 seconds
delay %victim1% 5
if (%aborted%)
   set message Aborted!
else

   set message Finished!
end
set message %message%  Delayers were:
foreach delayed victim
   set message %message% %victim.name%
done
echo %message%
~
[Script 12:93]
	Virtual:	:93
	Name:		Move'Em
	Type:		Mob
	Triggers:	Global Random 
	NumArg:		20
	Script:$
set room %random(32000)%
if (%roomexists(%room%)%)
    teleport %self% %room%
end
~
[Script 12:94]
	Virtual:	:94
	Name:		Ice Box Script
	Type:		Room
	Triggers:	Command 
	Argument:	quit qui qu q recall recal reca rec re r save sav sa s n e w d u
	NumArg:		3
	Script:$
Send %actor% Forget it.. your stuck here bitch.
teleport %actor% 1202
~
[Script 12:95]
	Virtual:	:95
	Name:		Fake Chat
	Type:		Object
	Triggers:	Command 
	Argument:	chat
	NumArg:		3
	Script:$
* Pretend to chat... <G>
if %actor.staff%
return 0
halt
end
send %actor% `y[`bCHAT`y] -> %arg%`n
return 1
~
[Script 12:97]
	Virtual:	:97
	Name:		prophet doll
	Type:		Object
	Triggers:	Command 
	Argument:	squeeze
	NumArg:		3
	Script:$
if %actor.targetobject(%arg%)% != %self%
    return 0
    halt
end

switch %random(10)%
    case 1
        send %actor% $%self%P says, 'Fricking $%actor%N.'
        break
    case 2
        send %actor% $%self%P says,  'cant fricking see anything cause you look like a damn fruit loop.'
        send %actor% $%self%P grins evilly at Godhand.
        break
    case 3
        send %actor% $%self%P says, 'Kylatia says she wants my hot little body.'
        break
    case 4
        send %actor% $%self%P says, 'You're fired!'
        send %actor% $%self%P goes off to fire someone.
        break
    case 5
        send %actor% $%self%P says, 'Abusing me will result in guilt-induced self-deletion.'
        break
    case 6
        send %actor% $%self%P says, 'Missions are like crack, they keep the players happy.'
        break
    case 7
        send %actor% $%self%P says, 'Oh, really? I think not.'
        send %actor% $%self%P slays you!
        damage %actor% 5
        send %actor% $%self%P rolls his eyes in disgust.
        break
    case 8
        send %actor% $%self%P says, 'Glad to know I'm intimidating while I'm in a pub drinking Guinness and eating meat-pies.'
        break
    case 9
        send %actor% $%self%P says, 'I'll just have to edit that rule.'
        break
    case 10
        echo `G  .''.             :         `n
        echo   :  :             :     ..  :
        echo `G  :'' '.''..''..''.:''. :..'':''`n
        echo   :    :   '..':  ::  '.'... '.'
        echo `G               :''`n
        break
done
~
[Script 12:98]
	Virtual:	:98
	Name:		mission flag oddjob script
	Type:		Object
	Triggers:	Command 
	Argument:	put flag
	NumArg:		2
	Script:$
return 0
zoneecho (%self.room% / 100) %actor.name% is trying to cheat!
~
BREAK
