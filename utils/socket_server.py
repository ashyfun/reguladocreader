import cgi

from datetime import datetime

from http.server import HTTPServer
from http.server import BaseHTTPRequestHandler


class Handler(BaseHTTPRequestHandler):
    def _set_headers(self):
        self.send_response(200)
        self.send_header('Content-Type', 'text/html')
        self.end_headers()

    def do_POST(self):
        self._set_headers()

        ctype, pdict = cgi.parse_header(self.headers['content-type'])
        length = int(self.headers['content-length'])

        print('content-type: ' + ctype + '; content-length: ' + str(length))

        data = self.rfile.read(length)
        datetime_now = datetime.now()

        file = open('logs/socket_server.%s.log' % (datetime_now.strftime('%Y-%m-%d_%H-%M-%S')), 'wb')
        file.write(data)
        file.close()


def server_run(server_class=HTTPServer, handler_class=Handler):
    host = '192.168.100.73'
    port = 5000

    print('Starting HTTP server on ' + host + ':' + str(port))

    httpd = server_class((host, port), handler_class)
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        httpd.server_close()


if __name__ == '__main__':
    server_run()
