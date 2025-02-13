import redis
import argparse
import pprint

if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument('-n', dest='asicName', action='store',
                        help='name of the ASIC to inspect')
    parser.add_argument('-s', dest='size', action='store',default=100,type=int,
                        help='size of ASIC bitfield')
        
    args = parser.parse_args()

    r=redis.Redis()
    b=r.bitfield(f'{args.asicName}')
    for offset in range(args.size):
        c=b.get('u8',f'#{offset}')
    toprint=[f'{i:02X}' for i in c.execute()]
    pprint.pprint(toprint,compact=1,width=150)
        
