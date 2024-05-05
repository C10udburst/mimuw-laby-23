-- Wireshark dissector
ppcb_proto = Proto("ppcb", "PPCB Protocol")
local f_type = ProtoField.string("ppcb.type", "Packet Type", FT_STRING)
local f_sid = ProtoField.uint64("ppcb.sid", "Session ID", base.HEX)
ppcb_proto.fields = {f_type, f_sid}
function ppcb_proto.dissector(buffer, pinfo, tree)
    pinfo.cols.protocol = "PPCB"
    local subtree = tree:add(ppcb_proto, buffer(), "PPCB Protocol Data")
    local offset = 0
    local packet_type = buffer(offset, 1):uint()
    offset = offset + 1
    subtree:add(f_sid, buffer(offset, 8))
    offset = offset + 8
    if packet_type == 1 then
        subtree:add(f_type, "CONN")
        pinfo.cols.info = "CONN"
        subtree:add(buffer(offset, 1), "Connection Type: " .. buffer(offset, 1):uint())
        offset = offset + 1
        subtree:add(buffer(offset, 8), "Length: " .. buffer(offset, 8):uint64())
    elseif packet_type == 2 then
        subtree:add(f_type, "CONNACC")
        pinfo.cols.info = "CONNACC"
    elseif packet_type == 3 then
        subtree:add(f_type, "CONRJT")
        pinfo.cols.info = "CONRJT"
    elseif packet_type == 4 then
        subtree:add(f_type, "DATA")
        subtree:add(buffer(offset, 8), "Packet ID: " .. buffer(offset, 8):uint64())
        pinfo.cols.info = "DATA " .. buffer(offset, 8):uint64()
        offset = offset + 8
        subtree:add(buffer(offset, 4), "Data Length: " .. buffer(offset, 4):uint())
    elseif packet_type == 5 then
        subtree:add(f_type, "ACC")
        subtree:add(buffer(offset, 8), "Packet ID: " .. buffer(offset, 8):uint64())
        pinfo.cols.info = "ACC " .. buffer(offset, 8):uint64()
    elseif packet_type == 6 then
        subtree:add(f_type, "RJT")
        pinfo.cols.info = "RJT"
        subtree:add(buffer(offset, 8), "Packet ID: " .. buffer(offset, 8):uint64())
    elseif packet_type == 7 then
        subtree:add(f_type, "RCVD")
        pinfo.cols.info = "RCVD"
        subtree:add(buffer(offset, 8), "Packet ID: " .. buffer(offset, 8):uint64())
    end

end

udp_table = DissectorTable.get("udp.port")
udp_table:add(21370,ppcb_proto)
tcp_table = DissectorTable.get("tcp.port")
tcp_table:add(21370,ppcb_proto)