
var s = prompt("Gain?", "1", "Set gain for all regions.");
if (s !== null) {

    var gain = parseFloat(s);

    var n = synthlab.tracks.count();
    console.log("Tracks:", n);
    for (var i = 0; i < n; i++) {
        var rc = synthlab.tracks.regionCount(i);
        for (var r = 0; r < rc; r++) {
            synthlab.actions.setRegionGain(i,r,gain);
        }
    }

}


    