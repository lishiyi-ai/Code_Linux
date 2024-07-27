print("run lua init.lua")
-- local sunnet
function OnInit(id)
    print("[lua] main OnInit id:"..id)

    local ping1 = sunnet.NewService("ping");
    print("[lua] new service ping1:"..ping1);

    local ping2 = sunnet.NewService("ping");
    print("[lua] new service ping2:"..ping2);

    local ping3 = sunnet.NewService("ping");
    print("[lua] new service ping3:"..ping3);

    sunnet.Send(ping1, ping3, "start");
    sunnet.Send(ping2, ping3, "start");
end

function OnExit()
    print("[lua] main OnExit")
end
