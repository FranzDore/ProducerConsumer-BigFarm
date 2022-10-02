#! /usr/bin/env python3
import sys, struct, socket, threading, time

HOST = "127.0.0.1"  # localhost
PORT = 65432 # porta d'ascolto

# codice da eseguire nei singoli thread 
class ClientThread(threading.Thread):
    def __init__(self, conn, addr, dizionario):
        threading.Thread.__init__(self)
        self.conn = conn
        self.addr = addr
        self.diz = dizionario
    def run(self):
      gestisci_connessione(self.conn, self.addr, self.diz)

def main(host=HOST, port=PORT):
  # creiamo il server socket
  dictionary = {}
  with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    try:  
      s.bind((host, port))
      s.listen()
      while True:
        # mi metto in attesa di una connessione
        conn, addr = s.accept()
        # lavoro con la connessione appena ricevuta
        # gestisci_connessione(conn,addr) 
        # t = threading.Thread(target=gestisci_connessione, args=(conn,addr))
        t = ClientThread(conn, addr, dictionary)
        t.start()
    except KeyboardInterrupt:
      pass
    print('Va bene smetto...')
    s.shutdown(socket.SHUT_RDWR)
    
def gestisci_connessione(conn, addr, diz): 
  with conn:  
    # ---- attendo due interi da 32 bit
    tipo = num_ricevi(conn, 4)    
    #CONTROLLO IL TIPO DI RICHIESTA
    if tipo == 1:
      print("Ricevuta richiesta 'Farm'. (Codice 1)")
      dim = num_ricevi(conn, 4) #Dimensione Nome File
      nomeFile = ''
      nomeFile = string_ricevi(conn, dim)
      dim = num_ricevi(conn, 4) #Dimensione Somma (strlen somma)
      somma = ''
      somma = string_ricevi(conn, dim)
      diz[nomeFile] = int(somma)
      print("Finito.")
    elif tipo == 2:
      print('Invio in corso. (Codice 2)')
      
      #Mando quante coppie esistono
      conn.sendall(struct.pack("!i", len(diz)))
  
      for w in sorted(diz, key=diz.get):
        conn.sendall(struct.pack("!i", len(w))) 
        conn.sendall(w.encode())
        temp = str(diz[w])
        conn.sendall(struct.pack("!i", len(temp)))
        conn.sendall(temp.encode())
      print("Finito.")
      
    elif tipo == 3:
      print('Invio in corso. (Codice 3)')
      lista = []
      quanti = num_ricevi(conn, 4) #Quanti numeri devo leggere
      for i in range(quanti):
        numero = ''
        dim = num_ricevi(conn, 4) #dimensione numero
        numero = string_ricevi(conn, dim) #intero
        numero = int(numero) 
        lista.append(numero)
      lista.sort()

      for z in lista:
        conteggio = 0
        for j in diz:
          if z == diz[j]:
            conteggio += 1
            conn.sendall(struct.pack("!i", len(j))) 
            conn.sendall(j.encode())
            temp = str(diz[j])
            conn.sendall(struct.pack("!i", len(temp)))
            conn.sendall(temp.encode()) 

        if conteggio == 0:
          answer = "Nessun File"
          conn.sendall(struct.pack("!i", len(answer))) 
          conn.sendall(answer.encode())
          
def recv_all(conn,n):
  chunks = b''
  bytes_recd = 0
  while bytes_recd < n:
    chunk = conn.recv(min(n - bytes_recd, 1024))
    if len(chunk) == 0:
      raise RuntimeError("socket connection broken")
    chunks += chunk
    bytes_recd = bytes_recd + len(chunk)
  return chunks

#def send_sorted(diz, conn):
  
  
def num_ricevi(conn,n): #Astraiamo un po' il codice
  temp = recv_all(conn, 4) 
  res = struct.unpack("!i", temp[:4])[0]
  return res

def string_ricevi(conn,n): #Astraiamo un po' il codice
  temp = recv_all(conn, n) 
  res = temp.decode('utf-8')
  return res

if len(sys.argv)==1:
  main()
else:
  print("Uso:\n\t %s [host] [port]" % sys.argv[0])
