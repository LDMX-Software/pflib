#!/usr/bin/env python
# coding: utf-8

# In[1]:


import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import mplhep as hep
import os 
import json


# In[2]:


class I2CConfig:
    def __init__(self):
        self.ETx_active = [0]  # Which eTx (transmitter) columns are active
        self.IdlePattern = '127ccc'  # Idle pattern to detect between packets (hex)
        self.HeaderMarker = '1e6'  # Header marker bits to identify packet start (hex)

i2c = I2CConfig()


# In[3]:


def to_hex(binary_str):
    """Convert binary string to hexadecimal string with appropriate width."""
    if not binary_str:
        return ''
    
    length_to_format = {
        16: '04X',
        24: '06X',
        32: '08X'
    }
    
    fmt = length_to_format.get(len(binary_str), '08X')
    return f'{int(binary_str, 2):{fmt}}'


to_hex_vectorized = np.vectorize(to_hex)


def parse_idle(idle_word):
    """
    Parse idle word into its constituent parts.
    
    Returns:
        pattern: Idle pattern (bits 8+)
        rr: Ready/Reset bits (bits 6-7)
        err: Error bits (bits 3-5)
        buff_stat: Buffer status (bits 0-2)
    """
    buff_stat = idle_word & 0x7
    err = (idle_word >> 3) & 0x7
    rr = (idle_word >> 6) & 0b11
    pattern = idle_word >> 8
    
    return pattern, rr, err, buff_stat


def parse_header_word0(header_word, return_dict=False):
    """
    Parse the first header word into its constituent fields.
    
    Args:
        header_word: 32-bit integer header word
        return_dict: If True, return dictionary; otherwise return tuple
    
    Returns:
        Dictionary or tuple of: HdrMarker, PayloadLength, P, E, HT, EBO, M, T, Hamming
    """
    hdr_marker = (header_word >> 23) & 0x1ff
    payload_length = (header_word >> 14) & 0x1ff
    p = (header_word >> 13) & 0x1
    e = (header_word >> 12) & 0x1
    ht = (header_word >> 10) & 0x3
    ebo = (header_word >> 8) & 0x3
    m = (header_word >> 7) & 0x1
    t = (header_word >> 6) & 0x1
    hamming = header_word & 0x3f
    
    if return_dict:
        return {
            "HdrMarker": hex(hdr_marker),
            "PayloadLength": payload_length,
            "P": p,
            "E": e,
            "HT": ht,
            "EBO": ebo,
            "M": m,
            "T": t,
            "Hamming": hamming
        }
    
    return hdr_marker, payload_length, p, e, ht, ebo, m, t, hamming


def parse_header_word1(header_word, return_dict=False):
    """
    Parse the second header word into its constituent fields.
    
    Args:
        header_word: 32-bit integer header word
        return_dict: If True, return dictionary; otherwise return tuple
    
    Returns:
        Dictionary or tuple of: BX, L1A, Orb, S, RR, CRC
    """
    bx = (header_word >> 20) & 0xfff
    l1a = (header_word >> 14) & 0x3f
    orb = (header_word >> 11) & 0x7
    s = (header_word >> 10) & 0x1
    rr = (header_word >> 8) & 0x3
    crc = header_word & 0xff
    
    if return_dict:
        return {
            "Bunch": bx,
            "Event": l1a,
            "Orbit": orb,
            "S": s,
            "RR": rr,
            "CRC": crc
        }
    
    return bx, l1a, orb, s, rr, crc


def convert_to_int(value):
    """
    Convert various types to integer, handling hex strings and numpy types.
    
    Args:
        value: String (hex), int, or numpy integer type
    
    Returns:
        Integer value
    
    Raises:
        TypeError: If value type is not supported
    """
    # Handle string types (including numpy string types)
    if isinstance(value, (str, np.str_, np.bytes_)):
        return int(value, 16)
    
    # Handle numeric types (including numpy integer types)
    if isinstance(value, (int, np.integer)):
        return int(value)
    
    raise TypeError(f"Unsupported type for conversion: {type(value)}")


def parse_header_words(header_words, return_dict=False):
    """
    Parse both header words and return combined fields.
    
    Args:
        header_words: Array-like containing two header words (can be hex strings or ints)
        return_dict: If True, return dictionary; otherwise return tuple
    
    Returns:
        Dictionary or tuple of all header fields
    """
    # Convert to integers if needed
    hdr_0 = convert_to_int(header_words[0])
    hdr_1 = convert_to_int(header_words[1])
    
    if return_dict:
        fields = parse_header_word0(hdr_0, return_dict=True)
        fields.update(parse_header_word1(hdr_1, return_dict=True))
        return fields
    
    return parse_header_word0(hdr_0) + parse_header_word1(hdr_1)


def parse_packet_header(packet_header0, packet_header1=0, as_hex=True, return_dict=False):
    """
    Parse packet header(s) into constituent fields.
    
    Args:
        packet_header0: First 32-bit packet header word
        packet_header1: Second 32-bit packet header word (optional, for full packet)
        as_hex: If True, return values as hex strings; otherwise as integers
        return_dict: If True, return dictionary; otherwise return tuple
    
    Returns:
        Dictionary or tuple of: Stat, Ham, F, CM0, CM1, E, ChMap
    """
    stat = (packet_header0 >> 29) & 0x7
    ham = (packet_header0 >> 26) & 0x7
    f = (packet_header0 >> 25) & 0x1
    cm0 = (packet_header0 >> 15) & 0x3ff
    cm1 = (packet_header0 >> 5) & 0x3ff
    e = (packet_header0 >> 4) & 0x1 if f == 1 else 0
    chmap = ((packet_header0 & 0x1f) << 32) + packet_header1
    
    if as_hex:
        stat = f'{stat:01x}'
        ham = f'{ham:01x}'
        f = f'{f:01x}'
        cm0 = f'{cm0:03x}'
        cm1 = f'{cm1:03x}'
        e = f'{e:01x}'
        chmap = f'{chmap:010x}'
    
    if return_dict:
        return {
            "Stat": stat,
            "Ham": ham,
            "F": f,
            "CM0": cm0,
            "CM1": cm1,
            "E": e,
            "ChMap": chmap
        }
    
    return stat, ham, f, cm0, cm1, e, chmap


# In[4]:


def parse_output_packets_no_idle(output_stream, header_marker='1e6'):
    """
    Parse output data stream into individual packets WITHOUT relying on idle words.
    Finds packets by detecting header markers and using PayloadLength.
    
    Args:
        output_stream: Array of hex strings (e.g., from df['link0'].values)
        header_marker: Hex string of header marker pattern (default: '1e6' for 0xf3xxxxxx)
    
    Returns:
        List of packets (each packet is an array of hex strings)
    """
    if isinstance(output_stream, np.ndarray):
        output_stream = output_stream.flatten()
    
    output_stream_int = np.array([int(x, 16) if isinstance(x, str) else x 
                                   for x in output_stream])
    
    # Find all header markers
    header_marker_int = int(header_marker, 16)
    is_header = (output_stream_int >> 23) == header_marker_int
    header_indices = np.where(is_header)[0]
    
    if len(header_indices) == 0:
        print("Warning: No header markers found in data stream")
        return []
    
    packets = []
    
    for i, start_idx in enumerate(header_indices):
        try:
            # Parse header to get payload length
            header_word0 = output_stream_int[start_idx]
            header_word1 = output_stream_int[start_idx + 1]
            
            # Extract payload length from header word 0
            payload_length = (header_word0 >> 14) & 0x1ff
            
            # Calculate packet size: 2 (header) + payload_length + 1 (CRC)
            packet_size = 2 + payload_length + 1
            
            # Check if we have enough data
            if start_idx + packet_size > len(output_stream):
                print(f"Warning: Packet at index {start_idx} truncated "
                      f"(needs {packet_size} words, only {len(output_stream) - start_idx} available)")
                break
            
            # Extract packet
            packet_end = start_idx + packet_size
            packet = output_stream[start_idx:packet_end]
            packets.append(packet)
            
        except (IndexError, ValueError) as e:
            print(f"Warning: Error parsing packet at index {start_idx}: {e}")
            break
    
    return packets


def parse_output_packets(df_output, i2c):
    """
    Parse output data stream into individual packets using idle word detection.
    
    Args:
        df_output: DataFrame containing output stream data
        i2c: Configuration object with ETx_active, IdlePattern, and HeaderMarker
    
    Returns:
        List of packets (each packet is an array of hex strings)
    """
    # Extract and flatten output stream from active eTx channels
    output_stream = df_output.values[:, i2c.ETx_active][:, ::-1].flatten()
    output_stream_int = np.vectorize(lambda x: int(x, 16))(output_stream)
    
    # Identify idle patterns and output headers
    idle_pattern = int(i2c.IdlePattern, 16)
    header_marker = int(i2c.HeaderMarker, 16)
    
    is_idle = (output_stream_int >> 8) == idle_pattern
    is_output_header = (output_stream_int >> 23) == header_marker
    
    # Packets start when we see a header after an idle
    is_packet_start = np.concatenate([[False], is_output_header[1:] & is_idle[:-1]])
    output_start_indices = np.where(is_packet_start)[0]
    
    # Extract individual packets
    packets = []
    for start_idx in output_start_indices:
        # Parse header to get payload length
        header_info = parse_header_word0(output_stream_int[start_idx])
        payload_length = header_info[1]  # PayloadLength is second element in tuple
        
        # Extract packet (header + payload)
        packet_end = start_idx + payload_length + 2
        packet = output_stream[start_idx:packet_end]
        packets.append(packet)
    
    return packets


# In[5]:


# Channel data compression code mappings
# Format: (code, bit_length, description, tctp_value)
COMPRESSION_CODES = {
    '0000': (24, 'ADCm1 and ADC', '00', True, True, False),   # has_adcm1, has_adc, has_toa
    '0001': (16, 'ADC only', '00', False, True, False),
    '0010': (24, 'ADCm1 and ADC', '01', True, True, False),
    '0011': (24, 'ADC and TOA', '00', False, True, True),
    '01':   (32, 'All data passing ZS', '00', True, True, True),
    '11':   (32, 'All data', '11', True, True, True),
    '10':   (32, 'Invalid code', '10', True, True, True),
}


def to_binary_32bit(value):
    """Convert hex string or int to 32-bit binary string."""
    if isinstance(value, str):
        return f'{int(value, 16):032b}'
    return f'{value:032b}'


def decode_channel_map(chmap_hex):
    """
    Decode channel map to get list of active channels.
    
    Args:
        chmap_hex: Hex string representing the 37-bit channel map
    
    Returns:
        List of active channel indices (0-36)
    """
    chmap_int = int(chmap_hex, 16)
    return [i for i in range(37) if (chmap_int >> (36 - i)) & 0x1]


def decode_compressed_channel(bin_string, passthrough=False):
    """
    Decode a single channel's compressed data.
    
    Args:
        bin_string: Binary string containing compressed channel data
        passthrough: If True, return raw 32 bits without decompression
    
    Returns:
        tuple: (decoded_data, bits_consumed, remaining_binary_string)
               decoded_data is 32-bit string: tctp(2) + adcm1(10) + adc(10) + toa(10)
    """
    if passthrough:
        return bin_string[:32], 32, bin_string[32:]
    
    # Determine compression code
    code = bin_string[:2]
    if code == '00':
        code = bin_string[:4]
    
    if code not in COMPRESSION_CODES:
        raise ValueError(f"Unknown compression code: {code}")
    
    bit_length, _, tctp, has_adcm1, has_adc, has_toa = COMPRESSION_CODES[code]
    code_len = len(code)
    
    # Initialize components
    adcm1 = '0' * 10
    adc = '0' * 10
    toa = '0' * 10
    
    # Extract data based on compression format
    offset = code_len
    
    if has_adcm1:
        adcm1 = bin_string[offset:offset+10]
        offset += 10
    
    if has_adc:
        adc = bin_string[offset:offset+10]
        offset += 10
    
    if has_toa:
        toa = bin_string[offset:offset+10]
    
    decoded_data = tctp + adcm1 + adc + toa
    remaining = bin_string[bit_length:]
    
    return decoded_data, bit_length, remaining


def unpack_single_packet(packet, active_links):
    """
    Unpack a single packet into its constituent data.
    
    Args:
        packet: Array of hex strings or integers representing the packet
        active_links: List of active eRx link indices (0-11)
    
    Returns:
        List of unpacked values in order: header fields, eRx data, channel data, CRC
    """
    # Initialize storage for all 12 eRx units and 37 channels each
    ch_data = np.full((12, 37), '', dtype=object)
    erx_header_data = np.full((12, 7), '', dtype=object)
    
    # Parse packet header
    header_info = parse_header_words(packet, return_dict=True)
    
    # Extract subpackets and CRC
    subpackets = packet[2:-1]
    crc = packet[-1]
    
    # Handle truncated packets
    if header_info['T'] == 1:
        assert len(subpackets) == 0, "Truncated packet should have no subpackets"
        combined_data = np.concatenate([erx_header_data, ch_data], axis=1).flatten()
        return list(header_info.values()) + list(combined_data) + [crc]
    
    # Convert subpackets to continuous binary string
    bin_string = ''.join(np.vectorize(to_binary_32bit)(subpackets))
    
    # Process each active eRx link
    for erx_idx in active_links:
        # Parse eRx header - first check F flag
        header_word0 = int(bin_string[:32], 2)
        f_flag = (header_word0 >> 25) & 0x1
        
        if f_flag == 0:
            # F=0: Full packet with 2-word header and channel data
            header_word1 = int(bin_string[32:64], 2)
            erx_header = parse_packet_header(header_word0, header_word1)
            erx_header_data[erx_idx] = erx_header
            bin_string = bin_string[64:]
        else:
            # F=1: Empty packet with 1-word header, no channel data, no ChMap
            erx_header = parse_packet_header(header_word0, 0)
            erx_header_data[erx_idx] = erx_header
            bin_string = bin_string[32:]
            # Skip channel processing for this eRx - no data!
            continue
        
        # Decode channel map to find active channels
        chmap_hex = erx_header[-1]  # ChMap is last element
        active_channels = decode_channel_map(chmap_hex)
        
        # Process each active channel
        bits_consumed = 0
        is_passthrough = header_info['P'] == 1
        
        for ch_idx in active_channels:
            decoded, bits_used, bin_string = decode_compressed_channel(
                bin_string, 
                passthrough=is_passthrough
            )
            ch_data[erx_idx][ch_idx] = decoded
            bits_consumed += bits_used
        
        # Handle padding to 32-bit boundary
        padding_bits = (32 - (bits_consumed % 32)) % 32
        
        # Verify padding is all zeros
        assert bin_string[:padding_bits] == '0' * padding_bits, \
            f"Expected {padding_bits} zero padding bits, got: {bin_string[:padding_bits]}"
        
        bin_string = bin_string[padding_bits:]
        
        # Verify remaining data is 32-bit aligned
        assert len(bin_string) % 32 == 0, \
            f"Remaining binary string not 32-bit aligned: {len(bin_string)} bits"
    
    # Combine all data into flat list
    combined_data = np.concatenate([erx_header_data, ch_data], axis=1).flatten()
    return list(header_info.values()) + list(combined_data) + [crc]


def unpack_packets(packet_list, active_links):
    """
    Unpack multiple packets into a DataFrame.
    
    Args:
        packet_list: List of packets (each packet is an array of hex strings/ints)
        active_links: List of active eRx link indices (0-11)
    
    Returns:
        DataFrame with columns for all header fields, eRx data, and channel data
    """
    unpacked_data = [unpack_single_packet(p, active_links) for p in packet_list]
    
    # Build column names
    columns = [
        'HeaderMarker', 'PayloadLength', 'P', 'E', 'HT', 'EBO', 
        'M', 'T', 'HdrHamming', 'BXNum', 'L1ANum', 'OrbNum', 
        'S', 'RR', 'HdrCRC'
    ]
    
    # Add columns for all 12 eRx units (even if not active)
    for i in range(12):
        # eRx header columns
        columns.extend([
            f'eRx{i:02d}_Stat', 
            f'eRx{i:02d}_Ham', 
            f'eRx{i:02d}_F',
            f'eRx{i:02d}_CM0', 
            f'eRx{i:02d}_CM1', 
            f'eRx{i:02d}_E', 
            f'eRx{i:02d}_ChMap'
        ])
        
        # Channel data columns (37 channels per eRx)
        columns.extend([f'eRx{i:02d}_ChData{j:02d}' for j in range(37)])
    
    columns.append('CRC')
    
    return pd.DataFrame(unpacked_data, columns=columns)


# In[31]:


def summarize_packet(result_df, event_idx=0):
    """
    Summarize the content of an unpacked packet DataFrame.
    
    Args:
        result_df: DataFrame from unpack_packets()
        event_idx: Which event/row to summarize (default: 0)
    """
    print("=" * 80)
    print(f"PACKET SUMMARY - Event {event_idx}")
    print("=" * 80)
    
    # Header Information
    print("\n HEADER INFORMATION:")
    print("-" * 80)
    header_cols = ['HeaderMarker', 'PayloadLength', 'P', 'E', 'HT', 'EBO', 
                   'M', 'T', 'HdrHamming', 'BXNum', 'L1ANum', 'OrbNum', 
                   'S', 'RR', 'HdrCRC']
    
    for col in header_cols:
        if col in result_df.columns:
            val = result_df[col].iloc[event_idx]
            print(f"  {col:15s}: {val}")
    
    # Check if truncated
    is_truncated = result_df['T'].iloc[event_idx] == 1
    if is_truncated:
        print("\n⚠️  TRUNCATED PACKET - No data available")
        return
    
    # eRx Summary - Only show eRx units with actual data
    print("\n" + "=" * 80)
    print("📡 eRx UNITS SUMMARY:")
    print("=" * 80)
    
    active_erx_list = []
    empty_erx_list = []
    
    for erx_idx in range(12):
        erx_prefix = f'eRx{erx_idx:02d}_'
        
        # Check if this eRx has any columns
        erx_cols = [col for col in result_df.columns if col.startswith(erx_prefix)]
        if not erx_cols:
            continue
        
        # Get F flag
        f_flag = result_df[f'{erx_prefix}F'].iloc[event_idx]
        
        if f_flag == '1':
            # Empty subpacket - just track it
            stat = result_df[f'{erx_prefix}Stat'].iloc[event_idx]
            ham = result_df[f'{erx_prefix}Ham'].iloc[event_idx]
            empty_erx_list.append((erx_idx, stat, ham))
            continue
        
        # Check if this eRx actually has channel data
        channels_with_data = []
        for ch in range(37):
            chdata_col = f'{erx_prefix}ChData{ch:02d}'
            chdata = result_df[chdata_col].iloc[event_idx]
            if chdata and chdata != '':
                channels_with_data.append(ch)
        
        if len(channels_with_data) == 0:
            # Has F=0 but no actual data - skip it
            continue
        
        active_erx_list.append(erx_idx)
        
        print(f"\n🔹 eRx {erx_idx:02d}:")
        print(f"   F flag: {f_flag} (HAS DATA)")
        
        # Full subpacket - show header info
        stat = result_df[f'{erx_prefix}Stat'].iloc[event_idx]
        ham = result_df[f'{erx_prefix}Ham'].iloc[event_idx]
        cm0 = result_df[f'{erx_prefix}CM0'].iloc[event_idx]
        cm1 = result_df[f'{erx_prefix}CM1'].iloc[event_idx]
        e = result_df[f'{erx_prefix}E'].iloc[event_idx]
        chmap = result_df[f'{erx_prefix}ChMap'].iloc[event_idx]
        
        print(f"   Status: {stat}")
        print(f"   Hamming: {ham}")
        
        # Handle empty CM values
        if cm0 and cm0 != '':
            print(f"   CM0 (Common Mode 0): {cm0} = {int(cm0, 16)}")
        else:
            print(f"   CM0 (Common Mode 0): (empty)")
        
        if cm1 and cm1 != '':
            print(f"   CM1 (Common Mode 1): {cm1} = {int(cm1, 16)}")
        else:
            print(f"   CM1 (Common Mode 1): (empty)")
        
        print(f"   Error: {e}")
        print(f"   ChMap: {chmap}")
        
        # Decode active channels from ChMap
        if chmap and chmap != '':
            chmap_int = int(chmap, 16)
            active_channels = [i for i in range(37) if (chmap_int >> (36 - i)) & 0x1]
        else:
            active_channels = []
        
        print(f"   Active channels ({len(active_channels)}): {active_channels}")
        
        # Check which channels actually have data
        channels_with_data = []
        for ch in range(37):
            chdata_col = f'{erx_prefix}ChData{ch:02d}'
            chdata = result_df[chdata_col].iloc[event_idx]
            if chdata and chdata != '':
                channels_with_data.append(ch)
        
        print(f"   Channels with data ({len(channels_with_data)}): {channels_with_data}")
        
        if len(channels_with_data) != len(active_channels):
            print(f"   ⚠️  WARNING: ChMap indicates {len(active_channels)} channels, "
                  f"but {len(channels_with_data)} have data!")
    
    # Summary at the end
    print(f"\n" + "-" * 80)
    if active_erx_list:
        print(f"✓ Active eRx units with data: {active_erx_list}")
    else:
        print(f"✗ No eRx units with channel data")
    
    if empty_erx_list:
        print(f"⊘ Empty eRx units (F=1): {[e[0] for e in empty_erx_list]}")
        for erx_idx, stat, ham in empty_erx_list:
            print(f"   eRx{erx_idx:02d}: Status={stat}, Hamming={ham}")



# In[7]:


def read_files(folder, filename=None):
    import glob
    headers = ["link0","link1","link2","link3","link4","link5","link6"]
    if filename:
        csv_files = [os.path.join(folder, filename)]
    else:
        csv_files = glob.glob(folder+"/*.csv")  # Update with your actual folder path

    df_list = [pd.read_csv(file, sep=" ", names=headers, skiprows=1) for file in csv_files]
    final_df = pd.concat(df_list, ignore_index=True)
    return final_df


# In[8]:


# data = read_files("data", "pedestal_20251208_115228_hexdump.csv")
data = read_files("data", "pedestal_prelevel_1.raw")
packet_data = data["link0"].values 

df_output = pd.DataFrame({'link0': packet_data})

# Use parseOutputPackets to split into individual packets
#packets = parse_output_packets(df_output, i2c)
packets = parse_output_packets_no_idle(packet_data, header_marker='1e6')

print(f"Found {len(packets)} packets")


# In[9]:


packet_data


# In[10]:


with open('run40unwrapped.txt', 'r') as f:
    lines = f.readlines()

hex_values = []
for line in lines:
    line = line.strip()
    # Check if line looks like a hex value (8 characters, all hex digits)
    if len(line) == 8 and all(c in '0123456789abcdefABCDEF' for c in line):
        hex_values.append(line.lower())

# Create numpy array with dtype=object
packet_data = np.array(hex_values, dtype=object)

print(packet_data)
#print(f"\nTotal hex values: {len(hex_values)}")


# In[32]:


packets = parse_output_packets_no_idle(packet_data, header_marker='1e6')

print(f"Found {len(packets)} packets")

# Show packet sizes
for i, pkt in enumerate(packets):
    print(f"Packet {i}: {len(pkt)} words")
    
# Unpack each packet
for i, pkt in enumerate(packets):
    print(f"\n{'='*80}")
    print(f"PACKET {i}")
    print(f"{'='*80}")
    result = unpack_packets([pkt], active_links=[9,10])
    summarize_packet(result, event_idx=0)


# In[12]:


parse_header_words([packets[0][0], packets[0][1]], return_dict=True)


# In[13]:


parse_header_words([packets[1][0], packets[1][1]], return_dict=True)


# In[ ]:


packets[0]


# In[14]:


packets[1]


# In[15]:


summarize_packet(result, event_idx=0)


# In[ ]:


import crcmod
import codecs

crc8 = crcmod.mkCrcFun(0x1a7, initCrc=0, xorOut=0, rev=False)

def calculate_crc8_bits(data):
    """
    Calculate 8-bit CRC using crcmod library.

    Args:
        data: 64-bit integer
    Returns:
        8-bit CRC value (integer)
    """
    # Convert integer to 8 bytes (hex string, then to bytes)
    hex_str = f'{data:016x}'
    byte_data = codecs.decode(hex_str, 'hex')
    crc_value = crc8(byte_data)
    
    return crc_value


def verify_header_crc(header_word0, header_word1, verbose=False):
    """
    Verify the 8-bit CRC in the event header.
    
    The CRC is calculated over a 64-bit value constructed as:
    [8 zeros][26 bits from word0, bits 6-31, excluding Hamming bits 0-5][6 zeros][24 bits from word1, bits 8-31, excluding CRC bits 0-7]
    
    Args:
        header_word0: First 32-bit header word (hex string or int)
        header_word1: Second 32-bit header word (hex string or int)
        verbose: If True, print detailed information
    
    Returns:
        tuple: (is_valid, transmitted_crc, calculated_crc)
    """
    # Convert to integers if needed
    if isinstance(header_word0, str):
        header_word0 = int(header_word0, 16)
    if isinstance(header_word1, str):
        header_word1 = int(header_word1, 16)
    
    # Extract transmitted CRC from header word 1 (bits 0-7)
    transmitted_crc = header_word1 & 0xFF
    
    # Build CRC calculation base (64 bits total):
    # Format: 8_zeros + 26_bits_from_word0 + 6_zeros + 24_bits_from_word1
    
    # Extract bits 6-31 from header_word0 (26 bits, excluding Hamming bits 0-5)
    word0_data = (header_word0 >> 6) & 0x3FFFFFF  # 26 bits
    
    # Extract bits 8-31 from header_word1 (24 bits, excluding CRC bits 0-7)
    word1_data = (header_word1 >> 8) & 0xFFFFFF   # 24 bits
    
    # Construct the 64-bit CRC base:
    # [8 zeros][26 bits from word0][6 zeros][24 bits from word1]
    header_crc_base = (word0_data << 30) | word1_data
    
    if verbose:
        print(f"Header Word 0:       0x{header_word0:08x}")
        print(f"                     0b{header_word0:032b}")
        print(f"                        {'D' * 26}{'H' * 6}  (D=Data, H=Hamming)")
        print()
        
        print(f"Header Word 1:       0x{header_word1:08x}")
        print(f"                     0b{header_word1:032b}")
        print(f"                        {'D' * 24}{'C' * 8}  (D=Data, C=CRC)")
        print()
        
        print("Building CRC Base (64-bit):")
        print(f"  Word 0 [31:6]:     0b{word0_data:026b} (26 bits)")
        print(f"  Word 1 [31:8]:     0b{word1_data:024b} (24 bits)")
        print()
        print(f"CRC Base Structure:")
        print(f"  0b{'0'*8}{word0_data:026b}{'0'*6}{word1_data:024b}")
        print(f"    {'8 zeros':<8}{'26 bits from W0':<26}{'6 zero':<6}{'24 bits from W1':<24}")
        print()
        print(f"CRC Base (64-bit):   0x{header_crc_base:016x}")
        print(f"                     0b{header_crc_base:064b}")
        print()
        print(f"Transmitted CRC:     0x{transmitted_crc:02x} = 0b{transmitted_crc:08b} = {transmitted_crc}")
    
    # Calculate CRC
    calculated_crc = calculate_crc8_bits(header_crc_base)
    
    if verbose:
        print(f"Calculated CRC:      0x{calculated_crc:02x} = 0b{calculated_crc:08b} = {calculated_crc}")
        print(f"CRC Valid:           {calculated_crc == transmitted_crc}")
    
    return calculated_crc == transmitted_crc, transmitted_crc, calculated_crc


def find_corrupted_bits(header_word0, header_word1):
    """
    Determine which bits would need to change to make the CRC match.
    Tests single-bit and two-bit errors.
    
    Args:
        header_word0: First 32-bit header word (hex string or int)
        header_word1: Second 32-bit header word (hex string or int)
    
    Returns:
        Dictionary with possible corrections
    """
    # Convert to integers
    if isinstance(header_word0, str):
        header_word0 = int(header_word0, 16)
    if isinstance(header_word1, str):
        header_word1 = int(header_word1, 16)
    
    transmitted_crc = header_word1 & 0xFF
    
    # Check if original is valid
    is_valid_orig, _, calc_crc_orig = verify_header_crc(header_word0, header_word1)
    
    results = {
        'original': {
            'word0': f'0x{header_word0:08x}',
            'word1': f'0x{header_word1:08x}',
            'transmitted_crc': transmitted_crc,
            'calculated_crc': calc_crc_orig,
            'is_valid': is_valid_orig
        },
        'single_bit_flips': [],
        'two_bit_flips': []
    }
    
    if is_valid_orig:
        print("Original CRC is already valid!")
        return results
    
    print(f"Original CRC mismatch: transmitted={transmitted_crc}, calculated={calc_crc_orig}")
    print("Searching for single-bit flips...")
    
    # Test single-bit flips in word 0 (bits 6-31, excluding Hamming bits 0-5)
    for bit in range(6, 32):
        test_word0 = header_word0 ^ (1 << bit)
        is_valid, _, calc_crc = verify_header_crc(test_word0, header_word1)
        if is_valid:
            results['single_bit_flips'].append({
                'location': f'Word0, bit {bit}',
                'corrected_word0': f'0x{test_word0:08x}',
                'corrected_word1': f'0x{header_word1:08x}',
                'original_bit': (header_word0 >> bit) & 1,
                'corrected_bit': (test_word0 >> bit) & 1,
                'crc': calc_crc
            })
    
    # Test single-bit flips in word 1 (bits 8-31, excluding CRC bits 0-7)
    for bit in range(8, 32):
        test_word1 = header_word1 ^ (1 << bit)
        is_valid, _, calc_crc = verify_header_crc(header_word0, test_word1)
        if is_valid:
            results['single_bit_flips'].append({
                'location': f'Word1, bit {bit}',
                'corrected_word0': f'0x{header_word0:08x}',
                'corrected_word1': f'0x{test_word1:08x}',
                'original_bit': (header_word1 >> bit) & 1,
                'corrected_bit': (test_word1 >> bit) & 1,
                'crc': calc_crc
            })
    
    # Only search two-bit flips if no single-bit solution found
    if not results['single_bit_flips']:
        print("No single-bit flip found, searching two-bit flips...")
        
        # Test two-bit flips in word 0
        for bit1 in range(6, 32):
            for bit2 in range(bit1 + 1, 32):
                test_word0 = header_word0 ^ (1 << bit1) ^ (1 << bit2)
                is_valid, _, calc_crc = verify_header_crc(test_word0, header_word1)
                if is_valid:
                    results['two_bit_flips'].append({
                        'location': f'Word0, bits {bit1} and {bit2}',
                        'corrected_word0': f'0x{test_word0:08x}',
                        'corrected_word1': f'0x{header_word1:08x}',
                        'crc': calc_crc
                    })
        
        # Test two-bit flips in word 1
        for bit1 in range(8, 32):
            for bit2 in range(bit1 + 1, 32):
                test_word1 = header_word1 ^ (1 << bit1) ^ (1 << bit2)
                is_valid, _, calc_crc = verify_header_crc(header_word0, test_word1)
                if is_valid:
                    results['two_bit_flips'].append({
                        'location': f'Word1, bits {bit1} and {bit2}',
                        'corrected_word0': f'0x{header_word0:08x}',
                        'corrected_word1': f'0x{test_word1:08x}',
                        'crc': calc_crc
                    })
        
        # Test one bit in each word
        print("Searching for one bit flip in each word...")
        for bit0 in range(6, 32):
            for bit1 in range(8, 32):
                test_word0 = header_word0 ^ (1 << bit0)
                test_word1 = header_word1 ^ (1 << bit1)
                is_valid, _, calc_crc = verify_header_crc(test_word0, test_word1)
                if is_valid:
                    results['two_bit_flips'].append({
                        'location': f'Word0 bit {bit0} and Word1 bit {bit1}',
                        'corrected_word0': f'0x{test_word0:08x}',
                        'corrected_word1': f'0x{test_word1:08x}',
                        'crc': calc_crc
                    })
    
    return results


# In[34]:


#header0 = "f313f007"
#header1 = "d631c0ba"

header0 = "f313f024"
header1 = "a0e22825"

is_valid, tx, calc = verify_header_crc(header0, header1, verbose=True)

if not is_valid:
    print("\n" + "=" * 80)
    print("Finding which bits need to change to fix CRC...")
    print("=" * 80)
    corrections = find_corrupted_bits(header0, header1)
    
    print(f"\nOriginal Header:")
    print(f"  Word 0: {corrections['original']['word0']}")
    print(f"  Word 1: {corrections['original']['word1']}")
    print(f"  Transmitted CRC: 0x{corrections['original']['transmitted_crc']:02x}")
    
    if corrections['single_bit_flips']:
        print(f"\n✓ Found {len(corrections['single_bit_flips'])} single-bit flip solution(s):")
        for i, fix in enumerate(corrections['single_bit_flips'], 1):
            print(f"\n  Solution {i}: Flip {fix['location']}")
            print(f"    Corrected Word 0: {fix['corrected_word0']}")
            print(f"    Corrected Word 1: {fix['corrected_word1']}")
            print(f"    CRC now matches: 0x{fix['crc']:02x}")
    elif corrections['two_bit_flips']:
        print(f"\n✓ Found {len(corrections['two_bit_flips'])} two-bit flip solution(s):")
        for i, fix in enumerate(corrections['two_bit_flips'][:5], 1):  # Show first 5
            print(f"\n  Solution {i}: Flip {fix['location']}")
            print(f"    Corrected Word 0: {fix['corrected_word0']}")
            print(f"    Corrected Word 1: {fix['corrected_word1']}")
            print(f"    CRC now matches: 0x{fix['crc']:02x}")
        if len(corrections['two_bit_flips']) > 5:
            print(f"\n  ... and {len(corrections['two_bit_flips']) - 5} more solutions")
    else:
        print("\n✗ No simple single or two-bit flip solution found")
        print("  This suggests multiple bit errors or a different error pattern")


# The hamming values are a parity check of different combinations of bits:
# 
# ```
# parityGroups = np.array([[56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41,
#                           40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26],
#                          [56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41,
#                           25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11],
#                          [56, 55, 54, 53, 52, 51, 50, 49, 40, 39, 38, 37, 36, 35, 34, 33,
#                           25, 24, 23, 22, 21, 20, 19, 18, 10,  9,  8,  7,  6,  5,  4],
#                          [56, 55, 54, 53, 48, 47, 46, 45, 40, 39, 38, 37, 32, 31, 30, 29,
#                           25, 24, 23, 22, 17, 16, 15, 14, 10,  9,  8,  7,  3,  2,  1],
#                          [56, 55, 52, 51, 48, 47, 44, 43, 40, 39, 36, 35, 32, 31, 28, 27,
#                           25, 24, 21, 20, 17, 16, 13, 12, 10,  9,  6,  5,  3,  2,  0],
#                          [56, 54, 52, 50, 48, 46, 44, 42, 40, 38, 36, 34, 32, 30, 28, 26,
#                           25, 23, 21, 19, 17, 15, 13, 11, 10,  8,  6,  4,  3,  1,  0]])
# ```
# for these different groups of bits, a Hamming code !=0 is whether there is an even or odd number of 1's in the values

# In[ ]:




