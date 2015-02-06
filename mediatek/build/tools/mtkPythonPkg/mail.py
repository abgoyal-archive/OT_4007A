#! /usr/bin/python

import sys
import os
import smtplib
import config
from email.mime.text import MIMEText

debug = False

class Mail(object):
    def __init__(self):
        self.mailHost = config.smtp['mailHost']
        self.mailUser = config.smtp['mailUser']

    def sendMail(self,mailTo,subject,content):
        msg = MIMEText(content)
        msg['Subject'] = subject
        msg['From'] = 'Yunbo Wang'
        msg['To'] = mailTo
        try:
            smtp = smtplib.SMTP()
            smtp.connect(self.mailHost)
            smtp.sendmail(self.mailUser,mailTo,msg.as_string())
            smtp.close()
            return True
        except Exception, e:
            print str(e)
            return False

    def debug(self):
        self.sendMail('yunbo.wang@mediatek.com','hello','this is python mail test')

if debug == True:
    mail = Mail()
    mail.debug()

