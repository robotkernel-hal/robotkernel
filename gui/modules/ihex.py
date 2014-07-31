def enum(**enums):
    return type('Enum', (), enums)

class ihex_data_record(object):
    def __init__(self, address, data):
        self.address = address
        self.data = data

class ihex(object):
    def __init__(self):
        self.data_records = {}
        self.types = enum(DATA_RECORD=0, EOF_RECORD=1, EXT_SEG_ADR_RECORD=2, 
                START_SEG_ADR_RECORD=3, EXT_LIN_ADR_RECORD=4, START_LIN_ADR_RECORD=5)
        self.values = {}
            
    def calc_checksum(self, line, checksum):
        actsum = 0
        length = len(line)/2

        for i in range(0,length):
            actsum = actsum + int(line[i*2:(i*2)+2], 16)

        actsum = actsum%256
        calcsum = ((actsum ^ 0xFF) + 1) % 256
        valid = (actsum+checksum)%256

        return (valid==0, calcsum)

    def load_file(self, fn):
        f = open(fn, 'r')
        upper_linear_base_address = 0
            
        for line in f:
            line = line.strip()
            checksum = int(line[-2:],16)
            #print 'set', checksum

            # skip non record lines
            if line[0] != ':':
                continue
            line = line[1:-2]
            

            valid, calcsum = self.calc_checksum(line, checksum)
            if not valid:
                print 'checksum invalid!'
            #print 'is valid %r, calcsum %X, storedsum %X' % (valid, calcsum, checksum)

            # 8-bit length of data in record
            reclen = int(line[0:2], 16)
            line = line[2:]

            # 16-bit load offset
            loadoffset = int(line[0:4], 16)
            line = line[4:]
            
            # 8-bit record type
            rectype = int(line[0:2], 16)
            line = line[2:]

            if rectype == self.types.EXT_LIN_ADR_RECORD:
                upper_linear_base_address = int(line[0:4], 16)<<16
            elif rectype == self.types.DATA_RECORD:
                data = line
                address = loadoffset + upper_linear_base_address
                self.data_records[address] = ihex_data_record(address, data)

                adr = address
                temp = data
                while len(temp):
                    self.values[adr] = int(temp[:4], 16)
                    adr += 2
                    temp = temp[4:]

    def write_file(self, fn):
        #print self.values
        #return
        f = open(fn, 'w+')
        upper_linear_base_address = 0
        chain_size = 32
        
        values = sorted(self.values)
        adr = 0
        line = ''
        for val in values:
            if val >= (adr + chain_size):
                if line:
                    line = '%02X%04X00%s' % (len(line)/2, adr%65536, line)
                    valid, checksum = self.calc_checksum(line, 0)
                    f.write(':%s%02X\n' % (line, checksum))

                adr = val
                line = '%04X' % self.values[val]
                if adr - (upper_linear_base_address << 16) > 65535:
                    # need extended addres line
                    upper_linear_base_address = adr >> 16
                    line2 = '02000004%04X' % (upper_linear_base_address)
                    valid, checksum = self.calc_checksum(line2, 0)
                    f.write(':%s%02X\n' % (line2, checksum))
            else:
                line = line + '%04X' % self.values[val]
        return

        for rec in sorted(self.data_records):
            record = self.data_records[rec]

            if record.address - (upper_linear_base_address << 16) > 65535:
                # need extended addres line
                upper_linear_base_address = record.address >> 16
                line = '02000004%04X' % (upper_linear_base_address)
                valid, checksum = self.calc_checksum(line, 0)
                f.write(':%s%02X\n' % (line, checksum))


            line = '%02X%04X00%s' % (len(record.data)/2, record.address%(65536), record.data)
            valid, checksum = self.calc_checksum(line, 0)
            f.write(':%s%02X\n' % (line, checksum))
        f.write(':00000001FF\n')

    def put_data(self, address, data):
        data_str = "".join(map(lambda x: "%04X" % x, data))
        self.data_records[address] = ihex_data_record(address, data_str)

        for i in range(0, len(data)):
            self.values[address + (i * 2)] = data[i]

    def get_lower_upper(self):
        tmp = sorted(self.values)
        return (tmp[0], tmp[-1])

    


            


