struct OfflinePlayHead : public juce::AudioPlayHead
{
    Optional<PositionInfo> getPosition() const override
    {
        // Atomic load: sehe entweder den ganzen alten oder den ganzen neuen State
        return infoSnapshot.load();
    }

    void setPositionInfo(int64_t samplePos, double sampleRate, double bpm)
    {
        PositionInfo local;  // lokale Kopie auf Stack
        
        double timeInSeconds = samplePos / sampleRate;
        double ppq = timeInSeconds * (bpm / 60.0);
        double beatsPerBar = 4.0;
        double ppqBarStart = std::floor(ppq / beatsPerBar) * beatsPerBar;

        local.setTimeInSamples(samplePos);
        local.setTimeInSeconds(timeInSeconds);
        local.setBpm(bpm);
        local.setTimeSignature(TimeSignature(4, 4));
        local.setPpqPosition(ppq);
        local.setPpqPositionOfLastBarStart(ppqBarStart);
        local.setIsLooping(false);
        local.setIsPlaying(playing.load());
        local.setIsRecording(recording.load());

        infoSnapshot.store(local);  // atomic swap, ein Aufruf
    }

    void setPlaying(bool p)   { playing.store(p); }
    void setRecording(bool r) { recording.store(r); }

private:
    std::atomic<PositionInfo> infoSnapshot;
    std::atomic<bool> playing { false };
    std::atomic<bool> recording { false };
};
