-- SpeechShare Native VST3 multitrack routing for REAPER.
-- Select 1-32 mono/stereo mic tracks. Builds a 2N bus and separate stems.
-- REQUIRES: SpeechShare Multitrack VST3; host pin count may limit N.
-- SAFETY: operate on a duplicate project. No SWS dependency.
local project=0
local n=reaper.CountSelectedTracks(project)
if n<1 or n>32 then reaper.MB('Select 1-32 mic tracks.', 'SpeechShare VST3', 0);return end
local source={}
for i=1,n do source[i]=reaper.GetSelectedTrack(project,i-1) end

local index=reaper.CountTracks(project)
reaper.Undo_BeginBlock()
reaper.PreventUIRefresh(1)
reaper.InsertTrackAtIndex(index,true)
local bus=reaper.GetTrack(project,index)
reaper.GetSetMediaTrackInfo_String(bus,'P_NAME','SpeechShare VST3 | AUTOMIX BUS',true)
reaper.SetMediaTrackInfo_Value(bus,'I_NCHAN',n*2)
reaper.SetMediaTrackInfo_Value(bus,'B_MAINSEND',0)
local fx=-1
for _,id in ipairs({'VST3: SpeechShare Multitrack','SpeechShare Multitrack'}) do
  fx=reaper.TrackFX_AddByName(bus,id,false,1)
  if fx>=0 then break end
end
if fx<0 then
  reaper.DeleteTrack(bus)
  reaper.PreventUIRefresh(-1)
  reaper.Undo_EndBlock('SpeechShare VST3 missing',-1)
  reaper.MB('SpeechShare Multitrack VST3 not found. Install/build it first. No original tracks were changed.','SpeechShare VST3',0)
  return
end
local _,pinsIn,pinsOut=reaper.TrackFX_GetIOSize(bus,fx)
if (pinsIn>0 and pinsIn<n*2) or (pinsOut>0 and pinsOut<n*2) then
  reaper.DeleteTrack(bus)
  reaper.PreventUIRefresh(-1)
  reaper.Undo_EndBlock('SpeechShare: unsupported bus pin count',-1)
  reaper.MB(('The host exposed only %d inputs / %d outputs. You selected %d microphones (%d channels).\n\nConfigure a larger multichannel VST3 bus in the host first, or test with 5 microphones. No original tracks were changed.'):format(pinsIn,pinsOut,n,n*2),'SpeechShare multichannel limitation',0)
  return
end
-- Force 1:1 pin mapping for all exposed channels. Reaper masks are 64-bit halves.
for i=0,n*2-1 do
  local low=i<32 and (1<<i) or 0
  local high=i>=32 and (1<<(i-32)) or 0
  reaper.TrackFX_SetPinMappings(bus,fx,0,i,low,high)
  reaper.TrackFX_SetPinMappings(bus,fx,1,i,low,high)
end
-- Set active-microphone maximum when the parameter is exposed by the plugin.
for i=0,reaper.TrackFX_GetNumParams(bus,fx)-1 do
  local ok,name=reaper.TrackFX_GetParamName(bus,fx,i)
  if ok and name=='Active microphones' then
    reaper.TrackFX_SetParamNormalized(bus,fx,i,(n-1)/31)
    break
  end
end
for i,src in ipairs(source) do
  local _,name=reaper.GetSetMediaTrackInfo_String(src,'P_NAME','',false)
  if name=='' then name=('Mic %02d'):format(i) end
  local sendIn=reaper.CreateTrackSend(src,bus)
  reaper.SetTrackSendInfo_Value(src,0,sendIn,'I_SENDMODE',0)
  reaper.SetTrackSendInfo_Value(src,0,sendIn,'I_SRCCHAN',0)
  reaper.SetTrackSendInfo_Value(src,0,sendIn,'I_DSTCHAN',(i-1)*2)
  reaper.SetTrackSendInfo_Value(src,0,sendIn,'D_VOL',1)
  reaper.SetMediaTrackInfo_Value(src,'B_MAINSEND',0)
  local last=reaper.CountTracks(project)
  reaper.InsertTrackAtIndex(last,true)
  local stem=reaper.GetTrack(project,last)
  reaper.GetSetMediaTrackInfo_String(stem,'P_NAME',('POST VST3 %02d | %s'):format(i,name),true)
  reaper.SetMediaTrackInfo_Value(stem,'I_NCHAN',2)
  local sendOut=reaper.CreateTrackSend(bus,stem)
  reaper.SetTrackSendInfo_Value(bus,0,sendOut,'I_SENDMODE',0)
  reaper.SetTrackSendInfo_Value(bus,0,sendOut,'I_SRCCHAN',(i-1)*2)
  reaper.SetTrackSendInfo_Value(bus,0,sendOut,'I_DSTCHAN',0)
  reaper.SetTrackSendInfo_Value(bus,0,sendOut,'D_VOL',1)
end
reaper.SetOnlyTrackSelected(bus)
reaper.TrackFX_Show(bus,fx,3)
reaper.TrackList_AdjustWindows(false)
reaper.UpdateArrange()
reaper.PreventUIRefresh(-1)
reaper.Undo_EndBlock(('SpeechShare VST3: create %d mic routing'):format(n),-1)
reaper.MB(('Created %d lavalier sources, %d stereo stems. Verify VST3 pin mapping and that the plugin shows MULTITRACK routing.'):format(n,n),'SpeechShare VST3 routing',0)
