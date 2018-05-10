from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy import Column
from sqlalchemy.types import Boolean, CHAR, Integer, String, DateTime, Enum, UnicodeText, TIMESTAMP
from sqlalchemy.sql import functions as func

BaseModel = declarative_base()


class Unimac(BaseModel):
    __tablename__ = 'unimac'

    id = Column(Integer, primary_key=True, nullable=False)
    mac = Column(CHAR(20), nullable=False)
    manu = Column(CHAR(20), nullable=False)
    station = Column(CHAR(20), nullable=False)
    start_time = Column(DateTime, index=True,  default=func.current_timestamp(), nullable=False)
    end_time = Column(DateTime, index=True,  default=func.current_timestamp(), nullable=False)
    mid_time = Column(TIMESTAMP,  default=func.current_timestamp(), nullable=False)
    flag = Column(UnicodeText)
    ap = Column(UnicodeText)
    ap_no_empty = Column(UnicodeText)

    def __repr__(self):
        return '<Unimac id : %r, mac : %r, start_time: %r, end_time: %r>' % (self.id, self.mac, self.start_time, self.end_time)

    def get_eval(self, attr, default=None):
        if hasattr(self, attr) and getattr(self, attr):
            return eval(getattr(self, attr))
        else:
            return default

class Init_sta1(BaseModel):
    """docstring for Initdata"""
    __tablename__ = 'init_sta1'

    id = Column(Integer, primary_key=True, nullable=False)
    mac = Column(CHAR(20), nullable=False)
    manu = Column(CHAR(20), nullable=False)
    apname = Column(CHAR(20))
    station = Column(CHAR(20), nullable=False)
    reqtime = Column(DateTime, nullable=False)
    newitem = Column(Boolean, index=True, nullable=False)
    
    def __repr__(self):
        return '<Init_sta1 id : %r, mac : %r, reqtime: %r>' % (self.id, self.mac, self.reqtime)


def init_db(engine):
    BaseModel.metadata.create_all(engine)


def drop_db(engine):
    BaseModel.metadata.drop_all(engine)