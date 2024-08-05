# Open on not_open UI

I wanted to show graphics or pictures in those desktop system, but it seems like for each time, you have to use c to compile your program again and again. Even if you did the compile, get the excutable, that software can't get running in other's computer. Others have to do the compile manually. Who designed this supid mechanism?

So I decided to do something. If I compile a simple UI software for each hardware and system in a format like "<computer_release_year>_<computer_name>_<system_name>_<system_version>_<system_archetecture>.run", anyone can just do a download and run, then send graphics or pictures to that software to display something, and even more, get mouse or keyboard input.

**This is not a static compile, because x11 library do not support static compile. All we could do is just compile a simple software for each archetecture or machine or system.**

## Why you do not use windows system?

I think if you make a xp software, it can run across win7,win8,win10, but what if you can't install xp system anymore because of hardware human_made limitation? What if starting from win_unknown version, they do not support xp software anymore? What if the virtualbox does not allow you to install xp system anymore? What if your user use arm_cpu?

## Why you do not use broswer?

Your broswer will do update automatically without asking you. For each time, they will restrict your freedom by adding more restriction on permissions. They say it is for protecting your machine, but that is a lie. For example, what if they ask you to add localhost port to a whitelist to be able to visit? They already made quite a lot of restrictions, for example, you can't visit your camera from http website. You can't visit "localhost:20000" from your broswer because the port is beyound development port like 8000 or 8080.

## Why you do not use frame_buffer?

If you can install old linux server, you can do it with frame buffer, but who knows if new hardware still allow you to install old linux server? Who knows if those people who controls linux server will use some new display protocol or not in the future? Just think about the android, they changed the display protocol, they do not use frame_buffer any more.

## The main reason I make this project?

I think they are evil because they did not even allow you to use your display device or input device or camera or microphone or speaker freely without install x GB dev dependencies. Even if you did, the software you make will not be able to get running in others computer unless they also install x GB garbage.

Or they have to install software from some deeply controlled app store. Like apt or snap or whatever.

## The root of the problem

We lack of freedom of speech. If every person has a free IP and domain and server. We could assign a person for a hardware or system or system with specific version, let that person handle the compability problem. In other words, each person maintain one binary excutable file for 'oepn on not_open ui', then the problem will be solved easyly.

## A better idea for company inner tools sharing

Suppose a company give every person a virtual_machine, like what you do in virtualbox. The system you use will always have a clone whenever someone wants to use it. It is like "yingshaoxo_private_machine" and "yingshaoxo_public_machine", the public one is always a instant copy of priviate machine.

Everyone in your company can visit and use your public machine, which is a system that has all your tools. You just have to avoid to login your private account in your work machine.

In this case, we have an open and stable work stack.
