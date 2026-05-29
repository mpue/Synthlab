// Full workflow: pick plugin -> create instrument track -> open
// Tracks tab -> create empty MIDI clip 1 bar long -> shift it one
// bar forward -> arm the track -> turn metronome on -> hit record.
var plugs = synthlab.plugins.list();
if (plugs.length === 0) { alert("No instrument plugins scanned."); }
else {
    var name = prompt("Plugin?", plugs[0]);
    if (name !== null) {
        var t = synthlab.actions.addInstrumentTrack(name);
        if (t < 0) { alert("Failed: " + name); }
        else {
            synthlab.ui.openTrackView();
            // 1 bar in 4/4 at current BPM, in seconds.
            var barSec = 4 * 60 / synthlab.transport.getBpm();
            var r = synthlab.actions.addMidiClip(t, 0, barSec);
            if (r >= 0) synthlab.actions.moveRegion(t, r, barSec);
            synthlab.tracks.setRecording(t, true);
            synthlab.transport.setMetronome(true);
            synthlab.transport.setRecording(true);
            synthlab.transport.play();
            console.log("Recording " + name + " on track " + t);
        }
    }
}
