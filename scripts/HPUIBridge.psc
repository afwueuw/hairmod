; SPDX-License-Identifier: GPL-3.0-or-later
Scriptname HPUIBridge Hidden
; Parts: 0 hair, 1 beard, 2 worn gear (FormID), 3 extra head part (type).
; IDs remain Ints or decimal strings, never lossy Floats.
String[] Function TargetsV2() Global
    Int gearCount = 0
    Int extraCount = 0
    if HairPositioner.IsReady()
        if HairPositioner.IsPartEnabled(2)
            gearCount = HairPositioner.GetGearCount()
        endif
        if HairPositioner.IsPartEnabled(3)
            extraCount = HairPositioner.GetExtraCount()
        endif
    endif
    String[] entries = Utility.CreateStringArray((gearCount + extraCount + 2) * 3)
    Int pos = 0
    Int part = 0
    while part < 2
        if HairPositioner.IsPartEnabled(part)
            entries[pos] = part as String
            entries[pos + 1] = "0"
            entries[pos + 2] = "Head part"
            pos += 3
        endif
        part += 1
    endwhile
    Int index = 0
    while index < extraCount
        Int extraType = HairPositioner.GetExtraType(index)
        if extraType != 0
            entries[pos] = "3"
            entries[pos + 1] = extraType as String
            entries[pos + 2] = HairPositioner.GetExtraName(index)
            pos += 3
        endif
        index += 1
    endwhile
    index = 0
    while index < gearCount
        Int itemID = HairPositioner.GetGearForm(index)
        if itemID != 0
            entries[pos] = "2"
            entries[pos + 1] = itemID as String
            entries[pos + 2] = HairPositioner.GetGearName(index)
            pos += 3
        endif
        index += 1
    endwhile
    return entries
EndFunction

Bool Function Available(Int part, Int itemID) Global
    if part < 0 || part > 3 || !HairPositioner.IsReady()
        return false
    endif
    if !HairPositioner.IsPartEnabled(part)
        return false
    endif
    if part < 2
        return HairPositioner.HasPart(part)
    endif
    Int index = 0
    if part == 2
        while index < HairPositioner.GetGearCount()
            if itemID != 0 && HairPositioner.GetGearForm(index) == itemID
                return true
            endif
            index += 1
        endwhile
    else
        while index < HairPositioner.GetExtraCount()
            if itemID != 0 && HairPositioner.GetExtraType(index) == itemID
                return true
            endif
            index += 1
        endwhile
    endif
    return false
EndFunction

Float Function ReadChannel(Int part, Int itemID, Int channel) Global
    if part == 2
        return HairPositioner.GetGearChannel(itemID, channel)
    elseif part == 3
        return HairPositioner.GetExtraChannel(itemID, channel)
    endif
    return HairPositioner.GetPartChannel(part, channel)
EndFunction

Int Function ReadPivot(Int part, Int itemID) Global
    if part == 2
        return HairPositioner.GetGearPivot(itemID)
    elseif part == 3
        return HairPositioner.GetExtraPivot(itemID)
    endif
    return HairPositioner.GetPartPivot(part)
EndFunction

; 0..8 channels, 9 pivot, 10 ready, 11 present, 12 enabled,
; 13 worn-toggle available, 14 worn-toggle value.
Float[] Function SnapshotV2(Int part, Int itemID) Global
    Float[] values = new Float[15]
    if !HairPositioner.IsReady() || part < 0 || part > 3
        return values
    endif
    values[10] = 1.0
    if HairPositioner.IsPartEnabled(part)
        values[12] = 1.0
    endif
    if !Available(part, itemID)
        return values
    endif
    values[11] = 1.0
    Int channel = 0
    while channel < 9
        values[channel] = ReadChannel(part, itemID, channel)
        channel += 1
    endwhile
    values[9] = ReadPivot(part, itemID) as Float
    if part == 0 && HairPositioner.HasWornToggle()
        values[13] = 1.0
        if HairPositioner.GetFollowWorn()
            values[14] = 1.0
        endif
    endif
    return values
EndFunction

String Function HexID(Int itemID) Global
    String digits = "0123456789ABCDEF"
    String result = ""
    Int nibbleIndex = 0
    while nibbleIndex < 8
        Int nibble = Math.LogicalAnd(Math.RightShift(itemID, (7 - nibbleIndex) * 4), 15)
        result += StringUtil.GetNthChar(digits, nibble)
        nibbleIndex += 1
    endwhile
    return result
EndFunction

Function MirrorTarget(Int part, Int itemID) Global
    String prefix = "HPOS_"
    if part == 1
        prefix = "HPOS_B_"
    elseif part == 2
        prefix = "HPOS_G_" + HexID(itemID) + "_"
    elseif part == 3
        prefix = "HPOS_X" + itemID + "_"
    endif
    Actor player = Game.GetPlayer()
    Int channel = 0
    while channel < 9
        NiOverride.SetBodyMorph(player, prefix + "C" + channel, "HairPositioner", ReadChannel(part, itemID, channel))
        channel += 1
    endwhile
    NiOverride.SetBodyMorph(player, prefix + "Pivot", "HairPositioner", ReadPivot(part, itemID) as Float)
    if part == 0 && HairPositioner.HasWornToggle()
        Float worn = 0.0
        if HairPositioner.GetFollowWorn()
            worn = 1.0
        endif
        NiOverride.SetBodyMorph(player, "HPOS_Worn", "HairPositioner", worn)
    endif
    ; Do not create HPOS_Present for a partial store. Without it RaceMenu
    ; seeds ALL native blocks on next open. Existing markers are preserved.
EndFunction

Function WriteValue(Int part, Int itemID, Int row, Float value) Global
    if !Available(part, itemID) || row < 0 || row > 10
        return
    endif
    if row < 9
        Float lo = -15.0
        Float hi = 15.0
        if row >= 6
            lo = 0.5
            hi = 2.0
        elseif row >= 3
            lo = -90.0
            hi = 90.0
        elseif part == 1
            lo = -10.0
            hi = 10.0
        endif
        if value < lo
            value = lo
        elseif value > hi
            value = hi
        endif
        if part == 2
            HairPositioner.SetGearChannel(itemID, row, value)
        elseif part == 3
            HairPositioner.SetExtraChannel(itemID, row, value)
        else
            HairPositioner.SetPartChannel(part, row, value)
        endif
    elseif row == 9
        Int pivot = value as Int
        if pivot < 0
            pivot = 0
        elseif pivot > 2
            pivot = 2
        endif
        if part == 2
            HairPositioner.SetGearPivot(itemID, pivot)
        elseif part == 3
            HairPositioner.SetExtraPivot(itemID, pivot)
        else
            HairPositioner.SetPartPivot(part, pivot)
        endif
    elseif part == 0 && HairPositioner.HasWornToggle()
        HairPositioner.SetFollowWorn(value >= 0.5)
    endif

    return
EndFunction

Float[] Function SetValueV2(Int part, Int itemID, Int row, Float value) Global
    if Available(part, itemID)
        WriteValue(part, itemID, row, value)
        MirrorTarget(part, itemID)
    endif
    return SnapshotV2(part, itemID)
EndFunction

; Restore all values as one bridge request; mirror once after all writes.
Float[] Function ApplyValuesV3(Int part, Int itemID, Float[] values) Global
    if values.Length != 11 || !Available(part, itemID)
        return SnapshotV2(part, itemID)
    endif
    Int row = 0
    while row < 11
        WriteValue(part, itemID, row, values[row])
        row += 1
    endwhile
    if Available(part, itemID)
        MirrorTarget(part, itemID)
    endif
    return SnapshotV2(part, itemID)
EndFunction
Float[] Function ResetTargetV2(Int part, Int itemID) Global
    if Available(part, itemID)
        if part == 2
            HairPositioner.ResetGear(itemID)
        elseif part == 3
            HairPositioner.ResetExtra(itemID)
        else
            HairPositioner.ResetPart(part)
        endif
        MirrorTarget(part, itemID)
    endif
    return SnapshotV2(part, itemID)
EndFunction

