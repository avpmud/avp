[Script 1]
	Virtual:	1
	Name:		medallion
	Type:		Mob
	Triggers:	Command 
	Argument:	touch
	NumArg:		1
	Script:$
if (%arg% == (office))
set dest 1210
elseif (%arg% == (enterance))
set dest 3001
else
send %actor% Touch what?
halt
end
echoaround %actor% %actor.name% touches the medallion.
send %actor% You touch the medallion.
echoaround %actor% %actor.name% disappears in a puff of smoke.
teleport %actor% %dest%
force %actor% look
echoaround %actor% %actor.name% appears in a blinding flash of light.
~
[Script 2]
	Virtual:	2
	Name:		SP's nodrop jail return
	Type:		Object
	Triggers:	Drop 
	NumArg:		100
	Script:$
teleport %actor% 22066
return 0
halt
end
~
[Script 3]
	Virtual:	3
	Name:		SP's nogive jail return
	Type:		Object
	Triggers:	Give 
	NumArg:		100
	Script:$
if %actor.level% <= 100
teleport %actor% 22066
halt
end
~
[Script 4]
	Virtual:	4
	Name:		noescape look script
	Type:		Object
	Triggers:	Command 
	Argument:	look
	NumArg:		3
	Script:$
teleport %actor% 22066
halt
end
~
[Script 5]
	Virtual:	5
	Name:		noescape l script
	Type:		Object
	Triggers:	Command 
	Argument:	l
	NumArg:		3
	Script:$
teleport %actor% 22066
halt
end
~
[Script 6]
	Virtual:	6
	Name:		noescape s script
	Type:		Object
	Triggers:	Command 
	Argument:	s
	NumArg:		3
	Script:$
teleport %actor% 22066
halt
end
~
[Script 7]
	Virtual:	7
	Name:		noescape n script
	Type:		Object
	Triggers:	Command 
	Argument:	n
	NumArg:		3
	Script:$
teleport %actor% 22066
halt
end
~
[Script 8]
	Virtual:	8
	Name:		noescape e script
	Type:		Object
	Triggers:	Command 
	Argument:	e
	NumArg:		3
	Script:$
teleport %actor% 22066
halt
end
~
[Script 9]
	Virtual:	9
	Name:		noescape w script
	Type:		Object
	Triggers:	Command 
	Argument:	w
	NumArg:		3
	Script:$
teleport %actor% 22066
halt
end
~
[Script 10]
	Virtual:	10
	Name:		Non-idle Recall
	Type:		Room
	Triggers:	Command 
	Argument:	*
	NumArg:		100
	Script:$
if (%actor.staff%)
return 1
elseif !(%actor.staff%) && (%actor.vnum% == -1)
send %actor% `y//`n Welcome back to the real world!
force %actor% recall
else
return 1
end
~
[Script 11]
	Virtual:	11
	Name:		balfhieh
	Type:		Mob
	Triggers:	Act 
	Argument:	climbs in
	Script:$
if (%self.room% == 1210 || %self.room% == 1224)
teleport %actor% 1240
wait 1s
send %actor% `RYou can't enter`n `w%self.room.name%`n `Rthat way!`n
send %actor% `cLet's try from here instead...`n
else
return 0
end
~
[Script 12]
	Virtual:	12
	Name:		balfhieh2
	Type:		Mob
	Triggers:	Act 
	Argument:	is pushed into the room
	Script:$
if (%self.room% == 1210 || %self.room% == 1224)
teleport %actor% 1240
wait 1s
send %actor% `RYou can't enter`n `w%self.room.name%`n`R that way.`n
else
return 0
end
~
[Script 13]
	Virtual:	13
	Name:		Hunger/Thirst
	Type:		Object
	Triggers:	Random 
	NumArg:		15
	Script:$
if (%self.owner% && !%self.owner.staff%)
    if (%random(100)% >= 50)
        send %self.owner% You are very hungry.
    else
        send %self.owner% You are very thirsty.
    end
end

~
[Script 14]
	Virtual:	14
	Name:		Hunger/Thirst Victim Setter
	Type:		Object
	Triggers:	Command 
	Argument:	hunger
	NumArg:		1
	Script:$
if (%actor.level% < 103)
    send %actor% Fuck you.
    halt
end
set victim %character(%arg.word(1)%)%
if (!%victim%)
    send %actor% Invalid victim.
else
    load obj 4
    move %loaded% %victim%
end
~
[Script 20]
	Virtual:	20
	Name:		practice guildmaster
	Type:		Mob
	Triggers:	Command 
	Argument:	practice
	Script:$
if (%actor.canbeseen%)
if (%actor.class% == %self.class%)
load m 20
force %actor% practice %arg%
purge systemgm
else
tell %actor% I can not train you.
end
else
say I do not deal with someone I can not see.
end
return 1
~
[Script 36]
	Virtual:	36
	Name:		new script
	Type:		Mob
	Triggers:	Global 
	Script:$
* Empty script
~
[Script 50]
	Virtual:	50
	Name:		Clan Heal Script (Hades)
	Type:		Room
	Triggers:	Global Random 
	NumArg:		100
	Script:$
heal all %random.50% + 150
wait 10
heal all %random.100% + 100
wait 10
heal all %random.150% + 50
wait 10
~
[Script 51]
	Virtual:	51
	Name:		leave-save
	Type:		Room
	Triggers:	Leave 
	NumArg:		100
	Script:$
if !(%actor.staff%) && (%actor.vnum% == -1)
force %actor% save
else
return 1
end
~
[Script 52]
	Virtual:	52
	Name:		quit-save
	Type:		Room
	Triggers:	Command 
	Argument:	quit
	NumArg:		100
	Script:$
if !(%actor.staff%)
force %actor% save
force %actor% quit
else
return 0
end
~
[Script 55]
	Virtual:	55
	Name:		damagetest
	Type:		Mob
	Triggers:	Command 
	Argument:	damage
	Script:$
echo Testing
damage %player(devon)% 500 %actor%
~
[Script 60]
	Virtual:	60
	Name:		cmdtest
	Type:		Room
	Triggers:	Command 
	Argument:	testscript
	Script:$
set commandlist 'echo This is just a test.' 'wait 5' 'echo Beware the test!' 'wait 5' 'echo Freee!!!'
while %commandlist%
    popfront commandlist command
echo Running commdn %command%
    %command%
done
~
[Script 65]
	Virtual:	65
	Name:		SARS
	Type:		Mob
	Triggers:	Random 
	NumArg:		5
	Script:$
if 0
detach 65 %self%
halt
end
if %random.100% > 5
    set victim %random.char%
    sneeze %victim%
    if %victim% && (%random.100% > 30)
         attach 65 %victim%
    end
    damage %self% 100
    if (%killed%)
        detach 65 %self%
    end
else
    sneeze
    send %self% You feel somewhat better.
    detach 65 %self%
end
~
[Script 66]
	Virtual:	66
	Name:		Bwahah
	Type:		Mob
	Triggers:	Random 
	NumArg:		50
	Script:$
log Script #66 attached to mob %self.vnum%.
~
[Script 67]
	Virtual:	67
	Name:		new script
	Type:		Mob
	Triggers:	Random 
	NumArg:		50
	Script:$
reward fearitself 5000
~
[Script 75]
	Virtual:	75
	Name:		Level Req Change Refund
	Type:		Object
	Triggers:	Load 
	NumArg:		100
	Script:$
#log Remove script 75 from object %self.vnum% (%self.name%`g)
halt
wait 1p

if (%self.owner% == 0 || %self.owner.npc% || %self.level% <= %self.owner.level%)
    halt
end

send %self.owner% %self.shortdesc% has had it's level requirement changed to %self.level%.
send %self.owner% You will be fully reimbursed for its initial cost.
send %self.owner% You gain %self.cost% MP!
mp %self.owner% %self.cost%
purge %self%
~
[Script 76]
	Virtual:	76
	Name:		test script
	Type:		Room
	Triggers:	Command 
	Argument:	test
	Script:$
echo %actor.name% is starting!
delay %actor% 10s
if %aborted%
    echo %actor.name% is an idiot!  %actor.heshe.cap% moved!
else
    echo %actor.name% is done!
end
~
[Script 90]
	Virtual:	90
	Name:		owhere
	Type:		Mob
	Triggers:	Command 
	Argument:	owhere
	Script:$
set itemname %arg.1%

set count 0
set room 0
while (%room% < 32768)
    if (%roomexists(%room%)%)
        set roomitems %room.iscontentsdeep(%itemname%)%
        set charlist %room.people%
        
        foreach charlist char
            pushback roomitems %char.iscarryingdeep(%itemname%)% %char.iswearingdeep(%itemname%)%
        done
        
        foreach roomitems item
            print line "%4.4d" %count%
            set line %line% %item.shortdesc%
            if (%item.inside%)
                set line %line%$_    ...inside $%item.inside%P
            end
            if (%item.owner%)
                set line %line%$_    ...carried by $%owner%N
            end
            set line %line% $_    ...in room %room%
            eval count %count% + 1
            send %actor% %line%
        done
    end
    eval room %room% + 1
done
~
[Script 94]
	Virtual:	94
	Name:		BAH
	Type:		Mob
	Triggers:	Command 
	Argument:	bah
	Script:$
set victim %character(superman)%
if !%victim%
    send %actor% Nope, not here.
else
send %victim% A small object falls from space and hits you in the chest!$_You pick it up..
end
~
[Script 96]
	Virtual:	96
	Name:		muzzle
	Type:		Object
	Triggers:	Command 
	Argument:	muzzle
	NumArg:		3
	Script:$
set target %character(%arg.1%)%
if (%actor.staff%)
move %self% %target%
force %target% save
send %actor% %target.name% successfully muzzled.
else
return 0
end
~
[Script 97]
	Virtual:	97
	Name:		WARNING eat
	Type:		Object
	Triggers:	Command 
	NumArg:		100
	Script:$
* Empty script
~
[Script 98]
	Virtual:	98
	Name:		WARNING drop
	Type:		Object
	Triggers:	Drop 
	NumArg:		100
	Script:$
if (%actor.level% <= 100) && !(%actor.staff%)
return 0
wait 1p
send %actor% You can't drop a warning! What the hell are you thinking?!
end
~
[Script 99]
	Virtual:	99
	Name:		WARNING give
	Type:		Object
	Triggers:	Give 
	NumArg:		100
	Script:$
if (%actor.level% <= 100) && (%victim.level% < 103)
return 0
wait 1p
send %actor% You can't just give it away, it's your warning.
end
~
BREAK
