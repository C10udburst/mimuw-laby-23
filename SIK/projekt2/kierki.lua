-- Wireshark dissector
kierki_proto = Proto("kierki", "Kierki Protocol")
local f_type = ProtoField.string("kierki.type", "Packet Type", FT_STRING)

-- common
local f_cards = ProtoField.string("kierki.cards", "Cards", FT_STRING)
local f_number = ProtoField.string("kierki.number", "Trick Number", FT_STRING)

-- IAM
local f_iam = ProtoField.string("kierki.iam.place", "Place", FT_STRING)

-- BUSY
local f_busy = ProtoField.string("kierki.taken.places", "Places", FT_STRING)

-- DEAL
local f_deal_type = ProtoField.string("kierki.deal.type", "Deal Type", FT_STRING)
local f_deal_first = ProtoField.string("kierki.deal.first", "First", FT_STRING)
-- f_cards

-- TRICK
-- f_cards
-- f_number

-- WRONG
-- f_number

-- TAKEN
-- f_number
-- f_cards
local f_taken_place = ProtoField.string("kierki.taken.place", "Place", FT_STRING)

-- SCORE
local f_score_1 = ProtoField.string("kierki.score.1", "Points 1", FT_STRING)
local f_dir_1 = ProtoField.string("kierki.dir.1", "Direction 1", FT_STRING)
local f_score_2 = ProtoField.string("kierki.score.2", "Points 2", FT_STRING)
local f_dir_2 = ProtoField.string("kierki.dir.2", "Direction 2", FT_STRING)
local f_score_3 = ProtoField.string("kierki.score.3", "Points 3", FT_STRING)
local f_dir_3 = ProtoField.string("kierki.dir.3", "Direction 3", FT_STRING)
local f_score_4 = ProtoField.string("kierki.score.4", "Points 4", FT_STRING)
local f_dir_4 = ProtoField.string("kierki.dir.4", "Direction 4", FT_STRING)

-- TOTAL
-- f_score_1
-- f_score_2
-- f_score_3
-- f_score_4

kierki_proto.fields = {f_type, f_cards, f_number, f_iam, f_busy, f_deal_type, f_deal_first, f_taken_place, f_score_1, f_score_2, f_score_3, f_score_4}

local possible_types = {
    "IAM",
    "BUSY",
    "DEAL",
    "TRICK",
    "WRONG",
    "TAKEN",
    "SCORE",
    "TOTAL"
}

function kierki_proto.dissector(buffer, pinfo, tree)
    pinfo.cols.protocol = "Kierki"
    local subtree = tree:add(kierki_proto, buffer(), "Kierki Protocol Data")
    local str = buffer():string()

    -- remove \r\n
    if str:sub(#str - 1) == "\r\n" then
        str = str:sub(1, #str - 2)
    else
        subtree:add_expert_info(PI_MALFORMED, PI_ERROR, "No \\r\\n at the end")
    end

    -- looking for type
    local found = ""
    for i, type in ipairs(possible_types) do
        if str:sub(1, #type) == type then
            subtree:add(f_type, buffer(0, #type))
            str = str:sub(#type + 1)
            found = type
            break
        end
    end

    if found == "" then
        subtree:add(f_type, "UNKNOWN")
        subtree:add_expert_info(PI_MALFORMED, PI_ERROR, "Unknown type")
        return
    end

    if str == "" then
        return
    end

    if found == "IAM" then
        subtree:add(f_iam, buffer(#found, #str))
    elseif found == "BUSY" then
        subtree:add(f_busy, buffer(#found, #str))
    elseif found == "DEAL" then
        subtree:add(f_deal_type, buffer(#found, 1))
        subtree:add(f_deal_first, buffer(#found + 1, 1))
        subtree:add(f_cards, buffer(#found + 2, #str))
    elseif found == "TRICK" then
        subtree:add(f_number, buffer(#found, 1))
        subtree:add(f_cards, buffer(#found + 1, #str))
    elseif found == "WRONG" then
        subtree:add(f_number, buffer(#found, #str))
    elseif found == "TAKEN" then
        local taken_place = str:sub(#str - 1)
        str = str:sub(1, #str - 2)
        local number = str:sub(1, 1)
        local cards = str:sub(2)
        subtree:add(f_number, buffer(#found, 1))
        subtree:add(f_cards, buffer(#found + 1, #str - 1))
        subtree:add(f_taken_place, buffer(#str, 1))
    elseif found == "SCORE" then
        local offset = 0
        subtree:add(f_dir_1, buffer(#found, 1))
        offset = offset + 1
        local score = str.gmatch(str:sub(offset), "%d+")
        subtree:add(f_score_1, buffer(#found + offset, #score))
        offset = offset + #score
        subtree:add(f_dir_2, buffer(#found + offset, 1))
        offset = offset + 1
        score = str.gmatch(str:sub(offset), "%d+")
        subtree:add(f_score_2, buffer(#found + offset, #score))
        offset = offset + #score
        subtree:add(f_dir_3, buffer(#found + offset, 1))
        offset = offset + 1
        score = str.gmatch(str:sub(offset), "%d+")
        subtree:add(f_score_3, buffer(#found + offset, #score))
        offset = offset + #score
        subtree:add(f_dir_4, buffer(#found + offset, 1))
        offset = offset + 1
        score = str.gmatch(str:sub(offset), "%d+")
        subtree:add(f_score_4, buffer(#found + offset, #score))
    elseif found == "TOTAL" then
        local offset = 0
        local score = str.gmatch(str, "%d+")
        subtree:add(f_score_1, buffer(#found, #score))
        offset = offset + #score
        score = str.gmatch(str:sub(offset), "%d+")
        subtree:add(f_score_2, buffer(#found + offset, #score))
        offset = offset + #score
        score = str.gmatch(str:sub(offset), "%d+")
        subtree:add(f_score_3, buffer(#found + offset, #score))
        offset = offset + #score
        score = str.gmatch(str:sub(offset), "%d+")
        subtree:add(f_score_4, buffer(#found + offset, #score))
    end
end

-- register our protocol
local tcp_port = DissectorTable.get("tcp.port")
for i = 21370, 21375 do
    tcp_port:add(i, kierki_proto)
end
-- add heuristic dissector if tcp stream starts with IAM, its probably kierki
kierki_proto:register_heuristic("tcp", function(buffer, pinfo, tree)
    local str = buffer():string()
    if str:sub(1, 3) == "IAM" then
        kierki_proto.dissector(buffer, pinfo, tree)
    end
    if str:sub(1, 4) == "BUSY" then
        kierki_proto.dissector(buffer, pinfo, tree)
    end
    if str:sub(1, 4) == "DEAL" then
        kierki_proto.dissector(buffer, pinfo, tree)
    end
    if str:sub(1, 5) == "TRICK" then
        kierki_proto.dissector(buffer, pinfo, tree)
    end
    if str:sub(1, 5) == "WRONG" then
        kierki_proto.dissector(buffer, pinfo, tree)
    end
    if str:sub(1, 5) == "TAKEN" then
        kierki_proto.dissector(buffer, pinfo, tree)
    end
    if str:sub(1, 5) == "SCORE" then
        kierki_proto.dissector(buffer, pinfo, tree)
    end
    if str:sub(1, 5) == "TOTAL" then
        kierki_proto.dissector(buffer, pinfo, tree)
    end
end)