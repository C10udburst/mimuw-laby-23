import sqlite3
import os.path
from dataclasses import dataclass
from typing import Generator, Optional
from datetime import datetime

@dataclass
class Tag:
    id: int
    name: str
    
    @staticmethod
    def from_row(row) -> 'Tag':
        return Tag(id=row[0], name=row[1])
    
@dataclass
class Image:
    id: int
    name: str
    description: Optional[str]
    pub_date: datetime
    
    @staticmethod
    def from_row(row) -> 'Image':
        return Image(id=row[0], name=row[1], description=row[2] if row[2] else None, pub_date=datetime.fromisoformat(row[3]))

class DjangoConnection():
    def __init__(self):
        self.path = os.path.join(os.path.dirname(__file__), '../project/db.sqlite3')
        self.conn = sqlite3.connect(self.path)
        self.cursor = self.conn.cursor()
        
    def __del__(self):
        self.conn.close()
        
    def get_tags(self) -> Generator[Tag, None, None]:
        self.cursor.execute("SELECT id, name FROM app_tag")
        for row in self.cursor.fetchall():
            yield Tag.from_row(row)
    
    def get_images(self) -> Generator[Image, None, None]:
        for row in self.cursor.execute("SELECT id, name, description, pub_date FROM app_image"):
            yield Image.from_row(row)
            
    def get_tagged_image(self, tag_id) -> Generator[Image, None, None]:
        # app_image_tags is a many-to-many table
        for row in self.cursor.execute("SELECT image_id, im.name, im.description, im.pub_date FROM app_image_tags it INNER JOIN app_image im ON it.image_id = im.id WHERE it.tag_id = ?", (tag_id,)):
            yield Image.from_row(row)
            
    def delete_image(self, image_id) -> Optional[Image]:
        deleted = self.cursor.execute("SELECT id, name, description, pub_date FROM app_image WHERE id = ?", (image_id,)).fetchone()
        if not deleted:
            return None
        deleted = Image.from_row(deleted)
        self.cursor.execute("DELETE FROM app_image WHERE id = ?", (image_id,))
        self.cursor.execute("DELETE FROM app_image_tags WHERE image_id = ?", (image_id,))
        self.conn.commit()
        return deleted
            