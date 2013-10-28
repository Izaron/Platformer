#include "level.h"

#include <iostream>
#include "tinyxml.h"


int Object::GetPropertyInt(std::string name)
{
    return atoi(properties[name].c_str());
}

float Object::GetPropertyFloat(std::string name)
{
    return strtod(properties[name].c_str(), NULL);
}

std::string Object::GetPropertyString(std::string name)
{
    return properties[name];
}

bool Level::LoadFromFile(std::string filename)
{
    TiXmlDocument levelFile(filename.c_str());

	// Загружаем XML-карту
    if(!levelFile.LoadFile())
    {
        std::cout << "Loading level \"" << filename << "\" failed." << std::endl;
        return false;
    }

	// Работаем с контейнером map
    TiXmlElement *map;
    map = levelFile.FirstChildElement("map");

	// Пример карты: <map version="1.0" orientation="orthogonal"
	// width="10" height="10" tilewidth="34" tileheight="34">
    width = atoi(map->Attribute("width"));
    height = atoi(map->Attribute("height"));
    tileWidth = atoi(map->Attribute("tilewidth"));
    tileHeight = atoi(map->Attribute("tileheight"));

	// Берем описание тайлсета и идентификатор первого тайла
    TiXmlElement *tilesetElement;
    tilesetElement = map->FirstChildElement("tileset");
    firstTileID = atoi(tilesetElement->Attribute("firstgid"));

	// source - путь до картинки в контейнере image
    TiXmlElement *image;
    image = tilesetElement->FirstChildElement("image");
    std::string imagepath = image->Attribute("source");

	// Пытаемся загрузить тайлсет
	sf::Image img;

    if(!img.loadFromFile(imagepath))
    {
        std::cout << "Failed to load tile sheet." << std::endl;
        return false;
    }

	// Очищаем карту от света (109, 159, 185)
	// Вообще-то в тайлсете может быть фон любого цвета, но я не нашел решения, как 16-ричную строку
	// вроде "6d9fb9" преобразовать в цвет
    img.createMaskFromColor(sf::Color(109, 159, 185));
	// Грузим текстуру из изображения
	tilesetImage.loadFromImage(img);
	// Расплывчатость запрещена
    tilesetImage.setSmooth(false);

	// Получаем количество столбцов и строк тайлсета
	int columns = tilesetImage.getSize().x / tileWidth;
    int rows = tilesetImage.getSize().y / tileHeight;

	// Вектор из прямоугольников изображений (TextureRect)
    std::vector<sf::Rect<int>> subRects;

	for(int y = 0; y < rows; y++)
	for(int x = 0; x < columns; x++)
	{
		sf::Rect<int> rect;

		rect.top = y * tileHeight;
		rect.height = tileHeight;
		rect.left = x * tileWidth;
		rect.width = tileWidth;

		subRects.push_back(rect);
	}

	// Работа со слоями
    TiXmlElement *layerElement;
    layerElement = map->FirstChildElement("layer");
    while(layerElement)
    {
        Layer layer;
		
		// Если присутствует opacity, то задаем прозрачность слоя, иначе он полностью непрозрачен
        if (layerElement->Attribute("opacity") != NULL)
        {
            float opacity = strtod(layerElement->Attribute("opacity"), NULL);
            layer.opacity = 255 * opacity;
        }
        else
        {
            layer.opacity = 255;
        }

		// Контейнер <data>
        TiXmlElement *layerDataElement;
        layerDataElement = layerElement->FirstChildElement("data");

        if(layerDataElement == NULL)
        {
            std::cout << "Bad map. No layer information found." << std::endl;
        }

		// Контейнер <tile> - описание тайлов каждого слоя
        TiXmlElement *tileElement;
        tileElement = layerDataElement->FirstChildElement("tile");

        if(tileElement == NULL)
        {
            std::cout << "Bad map. No tile information found." << std::endl;
            return false;
        }

        int x = 0;
        int y = 0;

        while(tileElement)
        {
            int tileGID = atoi(tileElement->Attribute("gid"));
            int subRectToUse = tileGID - firstTileID;

			// Устанавливаем TextureRect каждого тайла
            if (subRectToUse >= 0)
            {
                sf::Sprite sprite;
                sprite.setTexture(tilesetImage);
				sprite.setTextureRect(subRects[subRectToUse]);
                sprite.setPosition(x * tileWidth, y * tileHeight);
                sprite.setColor(sf::Color(255, 255, 255, layer.opacity));

                layer.tiles.push_back(sprite);
            }

            tileElement = tileElement->NextSiblingElement("tile");

            x++;
            if (x >= width)
            {
                x = 0;
                y++;
                if(y >= height)
                    y = 0;
            }
        }

        layers.push_back(layer);

        layerElement = layerElement->NextSiblingElement("layer");
    }

    // Работа с объектами
    TiXmlElement *objectGroupElement;

	// Если есть слои объектов
    if (map->FirstChildElement("objectgroup") != NULL)
    {
        objectGroupElement = map->FirstChildElement("objectgroup");
        while (objectGroupElement)
        {
			// Контейнер <object>
            TiXmlElement *objectElement;
            objectElement = objectGroupElement->FirstChildElement("object");
           
			while(objectElement)
            {
				// Получаем все данные - тип, имя, позиция, etc
                std::string objectType;
                if (objectElement->Attribute("type") != NULL)
                {
                    objectType = objectElement->Attribute("type");
                }
                std::string objectName;
                if (objectElement->Attribute("name") != NULL)
                {
                    objectName = objectElement->Attribute("name");
                }
                int x = atoi(objectElement->Attribute("x"));
                int y = atoi(objectElement->Attribute("y"));

				int width, height;

				sf::Sprite sprite;
                sprite.setTexture(tilesetImage);
				sprite.setTextureRect(sf::Rect<int>(0,0,0,0));
                sprite.setPosition(x, y);

				if (objectElement->Attribute("width") != NULL)
				{
					width = atoi(objectElement->Attribute("width"));
					height = atoi(objectElement->Attribute("height"));
				}
				else
				{
					width = subRects[atoi(objectElement->Attribute("gid")) - firstTileID].width;
					height = subRects[atoi(objectElement->Attribute("gid")) - firstTileID].height;
					sprite.setTextureRect(subRects[atoi(objectElement->Attribute("gid")) - firstTileID]);
				}

				// Экземпляр объекта
                Object object;
                object.name = objectName;
                object.type = objectType;
				object.sprite = sprite;

                sf::Rect <int> objectRect;
                objectRect.top = y;
                objectRect.left = x;
				objectRect.height = height;
				objectRect.width = width;
                object.rect = objectRect;

				// "Переменные" объекта
                TiXmlElement *properties;
                properties = objectElement->FirstChildElement("properties");
                if (properties != NULL)
                {
                    TiXmlElement *prop;
                    prop = properties->FirstChildElement("property");
                    if (prop != NULL)
                    {
                        while(prop)
                        {
                            std::string propertyName = prop->Attribute("name");
                            std::string propertyValue = prop->Attribute("value");

                            object.properties[propertyName] = propertyValue;

                            prop = prop->NextSiblingElement("property");
                        }
                    }
                }

				// Пихаем объект в вектор
                objects.push_back(object);

                objectElement = objectElement->NextSiblingElement("object");
            }
            objectGroupElement = objectGroupElement->NextSiblingElement("objectgroup");
        }
    }
    else
    {
        std::cout << "No object layers found..." << std::endl;
    }

    return true;
}

Object Level::GetObject(std::string name)
{
	// Только первый объект с заданным именем
    for (int i = 0; i < objects.size(); i++)
        if (objects[i].name == name)
            return objects[i];
}

std::vector<Object> Level::GetObjects(std::string name)
{
	// Все объекты с заданным именем
	std::vector<Object> vec;
    for(int i = 0; i < objects.size(); i++)
        if(objects[i].name == name)
			vec.push_back(objects[i]);

	return vec;
}

sf::Vector2i Level::GetTileSize()
{
	return sf::Vector2i(tileWidth, tileHeight);
}

void Level::Draw(sf::RenderWindow &window)
{
	// Рисуем все тайлы (объекты НЕ рисуем!)
	for(int layer = 0; layer < layers.size(); layer++)
		for(int tile = 0; tile < layers[layer].tiles.size(); tile++)
			window.draw(layers[layer].tiles[tile]);
}
