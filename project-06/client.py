# client.py
import socket
import json
from tinyec import registry
import hashlib
import random
from phe import paillier
from tinyec.ec import Point

HOST = '127.0.0.1'
PORT = 54545

# --- ECC helpers (same as server) ---
curve = registry.get_curve('secp256r1')

def hash_to_scalar(identifier: str):
    h = hashlib.sha256(identifier.encode()).digest()
    s = int.from_bytes(h, 'big')
    return s

def point_to_hex(pt):
    if pt is None:
        return {"inf": True}
    return {"x": format(pt.x, 'x'), "y": format(pt.y, 'x')}

def hex_to_point(d):
    if 'inf' in d:
        return None
    x = int(d["x"], 16)
    y = int(d["y"], 16)
    return Point(curve,x, y)

# --- socket helpers (same as server) ---
def send_json(conn, obj):
    data = json.dumps(obj).encode()
    conn.sendall(len(data).to_bytes(4, 'big') + data)

def recv_json(conn):
    ln_bytes = conn.recv(4)
    if not ln_bytes:
        raise ConnectionError("no data")
    ln = int.from_bytes(ln_bytes, 'big')
    data = b''
    while len(data) < ln:
        chunk = conn.recv(ln - len(data))
        if not chunk:
            raise ConnectionError("unexpected EOF")
        data += chunk
    return json.loads(data)

# --- client protocol ---
def run_client(user_ids):
    print("Client (P1) starting...")
    k1 = random.SystemRandom().randrange(2, curve.field.p - 1)
    print("P1 k1 chosen.")

    # Build ROUND1: for each identifier compute H(vi) -> s_i*G, then compute k1 * (s_i*G)
    p1_pts = []
    for vid in user_ids:
        s = hash_to_scalar(vid)
        pt = s * curve.g
        pt_k1 = k1 * pt
        p1_pts.append(point_to_hex(pt_k1))
    random.SystemRandom().shuffle(p1_pts)

    with socket.socket() as s:
        s.connect((HOST, PORT))
        # Send ROUND1
        send_json(s, {"type": "ROUND1", "points": p1_pts})
        print("Sent ROUND1 to server.")

        # Receive ROUND2
        msg = recv_json(s)
        if msg.get('type') != 'ROUND2':
            print("Protocol error: expecting ROUND2")
            return
        Z_hex = msg['Z']
        enc_pairs = msg['enc_pairs']
        paillier_n = int(msg['paillier_n'])
        print(f"Received ROUND2: |Z|={len(Z_hex)} enc_pairs={len(enc_pairs)}")

        # Reconstruct Paillier public key and encrypted numbers
        paillier_pub = paillier.PaillierPublicKey(paillier_n)

        # Convert Z to point set (bytes->point)
        Z_pts = set()
        for z_h in Z_hex:
            pt = hex_to_point(z_h)
            # Use tuple (x,y) as set key
            Z_pts.add( (pt.x, pt.y) )

        # For each enc_pair, compute k1*(H(w)^k2) and test membership in Z
        matched_enc = []
        for (pt_hex, enc_dict) in enc_pairs:
            pt_k2 = hex_to_point(pt_hex)
            # compute k1 * pt_k2 -> pt_k1k2
            pt_k1k2 = k1 * pt_k2
            key = (pt_k1k2.x, pt_k1k2.y)
            if key in Z_pts:
                # it's in intersection -> include enc
                # Rebuild EncryptedNumber: need ciphertext integer and exponent
                c = int(enc_dict['c'])
                exponent = int(enc_dict.get('exponent', 0))
                encnum = paillier.EncryptedNumber(paillier_pub, c, exponent)
                matched_enc.append(encnum)

        # homomorphically sum matched_enc
        if not matched_enc:
            enc_sum = paillier_pub.encrypt(0)
        else:
            acc = matched_enc[0]
            for e in matched_enc[1:]:
                acc = acc + e
            enc_sum = acc

        # Send ROUND3: encrypted sum (as c and exponent)
        send_json(s, {"type": "ROUND3", "enc_sum": {"c": enc_sum.ciphertext(), "exponent": enc_sum.exponent}})
        print("Sent ROUND3 (encrypted sum) to server.")
        # Done

if __name__ == "__main__":
    # demo user ids
    user_ids = [
        "pw:alice123",
        "pw:qwerty",
        "pw:letmein",
        "pw:unique_pass_42"
    ]
    run_client(user_ids)
