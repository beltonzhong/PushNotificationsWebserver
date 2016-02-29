import java.io.*;
import java.net.*;

public class RESTServer {
  public static void main(String[] args) throws Exception {
    // cr�ation de la socket
    int port = 8001;
    ServerSocket serverSocket = new ServerSocket(port);
    System.err.println("Server started on port: " + port);

    // repeatedly wait for connections, and process
    while (true) {
      // on reste bloqu� sur l'attente d'une demande client
      Socket clientSocket = serverSocket.accept();
      System.err.println("New connection accepted");

      // on ouvre un flux de converation

      BufferedReader in = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
      BufferedWriter out = new BufferedWriter(new OutputStreamWriter(clientSocket.getOutputStream()));

      // chaque fois qu'une donn�e est lue sur le r�seau on la renvoi sur
      // le flux d'�criture.
      // la donn�e lue est donc retourn�e exactement au m�me client.
      String s;
      while ((s = in.readLine()) != null) {
          System.out.println(s);
          if (s.isEmpty()) {
              break;
          }
      }

      out.write("HTTP/1.0 200 OK\r\n");
      out.write("Date: Fri, 31 Dec 1999 23:59:59 GMT\r\n");
      out.write("Server: Apache/0.8.4\r\n");
      out.write("Content-Type: text/html\r\n");
      out.write("Expires: Sat, 01 Jan 2000 00:59:59 GMT\r\n");
      out.write("Last-modified: Fri, 09 Aug 1996 14:21:40 GMT\r\n");
      out.write("\r\n");
      out.write("<TITLE>Example</TITLE>");
      out.write("<P>This is an example page.</P>");

      // on ferme les flux.
      System.err.println("Connection ended");
      out.close();
      in.close();
      clientSocket.close();
    }
  }
}