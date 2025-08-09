# server.py
import socket
import json
from tinyec import registry
import hashlib
import random
from phe import paillier
from tinyec.ec import Point

HOST = '127.0.0.1'
PORT = 54545

# --- ECC helpers using tinyec ---
curve = registry.get_curve('secp256r1')  # P-256 (secp256r1)

def hash_to_scalar(identifier: str):
    h = hashlib.sha256(identifier.encode()).digest()
    s = int.from_bytes(h, 'big')
    return s

def scalar_mul_G(scalar: int):
    # produce scalar * G (returns tinyec Point)
    return scalar * curve.g

def point_to_hex(pt):
    # convert tinyec Point to tuple hex strings (x,y) (use None for infinity)
    if pt is None:
        return {"inf": True}
    return {"x": format(pt.x, 'x'), "y": format(pt.y, 'x')}

def hex_to_point(d):
    if 'inf' in d:
        return None
    x = int(d["x"], 16)
    y = int(d["y"], 16)
    return Point(curve,x, y)

# --- socket helpers ---
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

# --- main service logic ---
def run_server(leaked_pairs):
    # leaked_pairs: list of (identifier string, count int)
    print("Server (P2) starting...")
    # P2 chooses k2
    k2 = random.SystemRandom().randrange(2, curve.field.p - 1)
    print("P2 k2 chosen.")

    # Paillier keypair
    paillier_pub, paillier_priv = paillier.generate_paillier_keypair()
    print("Paillier keypair generated (server).")

    with socket.socket() as s:
        s.bind((HOST, PORT))
        s.listen(1)
        print(f"Listening on {HOST}:{PORT} ... (waiting for client)")
        conn, addr = s.accept()
        with conn:
            print("Connected by", addr)
            # --- Receive ROUND1 from P1: list of points (each as hex dict) ---
            msg = recv_json(conn)
            if msg.get('type') != 'ROUND1':
                print("Protocol error: expecting ROUND1")
                return
            p1_points_hex = msg['points']  # list of hex dicts
            p1_points = [hex_to_point(d) for d in p1_points_hex]
            print(f"Received ROUND1 with {len(p1_points)} points from P1.")

            # --- Round2 preparation ---
            # Compute Z = { k2 * (points received) } -> send back to P1
            Z_pts = [k2 * pt for pt in p1_points]  # tinyec supports scalar*Point
            Z_hex = [point_to_hex(pt) for pt in Z_pts]

            # For each leaked pair, compute H(w)^k2 and Paillier-encrypt t_j
            enc_pairs = []
            for (wj, tj) in leaked_pairs:
                s_w = hash_to_scalar(wj)
                # point s_w * G, then multiply by k2 => k2 * (s_w * G) = (k2*s_w)*G
                pt_w_k2 = (k2 * (s_w * curve.g))
                enc_tj = paillier_pub.encrypt(tj)
                enc_pairs.append( (point_to_hex(pt_w_k2), {"c": enc_tj.ciphertext(), "exponent": enc_tj.exponent}) )

            # Shuffle both sets (to hide ordering)
            random.SystemRandom().shuffle(Z_hex)
            random.SystemRandom().shuffle(enc_pairs)

            # Send ROUND2: Z and enc_pairs and public key n
            payload = {
                "type": "ROUND2",
                "Z": Z_hex,
                "enc_pairs": enc_pairs,
                "paillier_n": paillier_pub.n
            }
            send_json(conn, payload)
            print("Sent ROUND2 to P1.")

            # --- Receive ROUND3: encrypted sum from P1 ---
            msg3 = recv_json(conn)
            if msg3.get('type') != 'ROUND3':
                print("Protocol error: expecting ROUND3")
                return
            encsum_dict = msg3['enc_sum']  # {c, exponent}
            encsum = paillier.EncryptedNumber(paillier_pub, int(encsum_dict['c']), int(encsum_dict.get('exponent', 0)))
            SJ = paillier_priv.decrypt(encsum)
            print("Server decrypted intersection-sum SJ =", SJ)
            # done
            return

if __name__ == "__main__":
    # small demo leaked set (identifier strings and counts)
    leaked = [
        ("pw:qwerty", 1000),
        ("pw:123456", 5000),
        ("pw:letmein", 800),
        ("pw:password", 3000)
    ]
    run_server(leaked)
