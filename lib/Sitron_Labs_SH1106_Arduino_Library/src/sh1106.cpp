/* Self header */
#include "sh1106.h"

/**
 *
 * @param[in] i2c_library
 * @param[in] i2c_address
 * @param[in] pin_res
 * @param[in] buffer A pointer to the buffer that will be used to store a local copy of the gdram, should be (m_active_width * (m_active_height / 8)) bytes.
 */
int sh1106::setup(TwoWire& i2c_library, const uint8_t i2c_address, const int pin_res, uint8_t* const buffer) {

    /* Ensure i2c address is valid */
    if (i2c_address != 0x3C && i2c_address != 0x3D) {
        return -EINVAL;
    }

    /* Save parameters */
    m_interface = INTERFACE_I2C_BUFFERED;
    m_i2c_library = &i2c_library;
    m_i2c_address = i2c_address;
    m_buffer = buffer;

    /* Perform reset */
    pinMode(pin_res, OUTPUT);
    digitalWrite(pin_res, LOW);
    delay(1);
    digitalWrite(pin_res, HIGH);
    delay(1);

    /* Configure panel */
    int res = 0;
    res |= command_send(COMMAND_DISPLAY_OFF);
    res |= command_send(COMMAND_FREQUENCY_SET, 0x80);
    res |= command_send(COMMAND_MULTIPLEX_SET, m_active_height - 1);
    res |= command_send(COMMAND_DISPLAY_OFFSET_SET, 0x00);
    res |= command_send(COMMAND_STARTLINE_SET | 0x00);
    res |= command_send(COMMAND_CHARGEPUMP_SET, 0x14);
    res |= command_send(COMMAND_MEMORYMODE_SET, 0x00);
    res |= command_send(COMMAND_SEGREMAP_SET | 0x01);
    res |= command_send(COMMAND_SCANDIRECTION_DECREASING);
    res |= command_send(COMMAND_PADS_CONFIGURATION, 0x12);
    res |= command_send(COMMAND_CONTRAST_SET, 0x80);
    res |= command_send(COMMAND_PRECHARGE_PERIOD_SET, 0xF1);
    res |= command_send(COMMAND_VCOMH_DESELECT_LEVEL_SET, 0x40);
    res |= command_send(COMMAND_ENTIREON_DISABLED);
    res |= command_send(COMMAND_INVERSION_DISABLED);
    for (size_t i = 0; i < (m_gdram_height + 7) / 8; i++) {
        res |= command_send(COMMAND_PAGE_ADDRESS + i);
        res |= command_send(COMMAND_COLUMN_ADDRESS_L | (2 & 0x0F));
        res |= command_send(COMMAND_COLUMN_ADDRESS_H | (2 >> 4));
        for (size_t j = 0; j < m_active_width; j++) {
            res |= data_send(0x00);
        }
    }
    res |= command_send(COMMAND_DISPLAY_ON);
    if (res != 0) {
        return -EIO;
    }

    /* Return success */
    return 0;
}

/**
 *
 * @param[in] i2c_library
 * @param[in] i2c_address
 * @param[in] pin_res
 */
int sh1106::setup(TwoWire& i2c_library, const uint8_t i2c_address, const int pin_res) {

    /* Ensure i2c address is valid */
    if (i2c_address != 0x3C && i2c_address != 0x3D) {
        return -EINVAL;
    }

    /* Save parameters */
    m_interface = INTERFACE_I2C_LIGHT;
    m_i2c_library = &i2c_library;
    m_i2c_address = i2c_address;
    m_buffer = NULL;

    /* Perform reset */
    pinMode(pin_res, OUTPUT);
    digitalWrite(pin_res, LOW);
    delay(1);
    digitalWrite(pin_res, HIGH);
    delay(1);

    /* Configure driver */
    int res = 0;
    res |= command_send(COMMAND_DISPLAY_OFF);
    res |= command_send(COMMAND_FREQUENCY_SET, 0x80);
    res |= command_send(COMMAND_MULTIPLEX_SET, m_active_height - 1);
    res |= command_send(COMMAND_DISPLAY_OFFSET_SET, 0x00);
    res |= command_send(COMMAND_STARTLINE_SET | 0x00);
    res |= command_send(COMMAND_CHARGEPUMP_SET, 0x14);
    res |= command_send(COMMAND_MEMORYMODE_SET, 0x00);
    res |= command_send(COMMAND_SEGREMAP_SET | 0x01);
    res |= command_send(COMMAND_SCANDIRECTION_DECREASING);
    res |= command_send(COMMAND_PADS_CONFIGURATION, 0x12);
    res |= command_send(COMMAND_CONTRAST_SET, 0x80);
    res |= command_send(COMMAND_PRECHARGE_PERIOD_SET, 0xF1);
    res |= command_send(COMMAND_VCOMH_DESELECT_LEVEL_SET, 0x40);
    res |= command_send(COMMAND_ENTIREON_DISABLED);
    res |= command_send(COMMAND_INVERSION_DISABLED);
    for (size_t i = 0; i < (m_gdram_height + 7) / 8; i++) {
        res |= command_send(COMMAND_PAGE_ADDRESS + i);
        res |= command_send(COMMAND_COLUMN_ADDRESS_L | (2 & 0x0F));
        res |= command_send(COMMAND_COLUMN_ADDRESS_H | (2 >> 4));
        for (size_t j = 0; j < m_active_width; j++) {
            res |= data_send(0x00);
        }
    }
    res |= command_send(COMMAND_DISPLAY_ON);
    if (res != 0) {
        return -EIO;
    }

    /* Return success */
    return 0;
}

/**
 *
 * @param[in] spi_library
 * @param[in] spi_speed
 * @param[in] pin_cs
 * @param[in] pin_dc
 * @param[in] pin_res
 * @param[in] buffer A pointer to the buffer that will be used to store a local copy of the gdram, should be (m_active_width * (m_active_height / 8)) bytes.
 */
int sh1106::setup(SPIClass& spi_library, const int spi_speed, const int pin_cs, const int pin_dc, const int pin_res, uint8_t* const buffer) {

    /* Ensure spi speed is within supported range */
    if (spi_speed > 2000000) {
        return -EINVAL;
    }

    /* Save parameters */
    m_interface = INTERFACE_SPI_4WIRES;
    m_spi_library = &spi_library;
    m_spi_settings = SPISettings(spi_speed, MSBFIRST, SPI_MODE0);
    m_pin_cs = pin_cs;
    m_pin_dc = pin_dc;
    m_buffer = buffer;

    /* Configure gpios */
    digitalWrite(m_pin_cs, HIGH);
    pinMode(m_pin_cs, OUTPUT);
    pinMode(m_pin_dc, OUTPUT);

    /* Perform reset */
    pinMode(pin_res, OUTPUT);
    digitalWrite(pin_res, LOW);
    delay(1);
    digitalWrite(pin_res, HIGH);
    delay(1);

    /* Configure panel */
    int res = 0;
    res |= command_send(COMMAND_DISPLAY_OFF);
    res |= command_send(COMMAND_FREQUENCY_SET, 0x80);
    res |= command_send(COMMAND_MULTIPLEX_SET, m_active_height - 1);
    res |= command_send(COMMAND_DISPLAY_OFFSET_SET, 0x00);
    res |= command_send(COMMAND_STARTLINE_SET | 0x00);
    res |= command_send(COMMAND_CHARGEPUMP_SET, 0x14);
    res |= command_send(COMMAND_MEMORYMODE_SET, 0x00);
    res |= command_send(COMMAND_SEGREMAP_SET | 0x01);
    res |= command_send(COMMAND_SCANDIRECTION_DECREASING);
    res |= command_send(COMMAND_PADS_CONFIGURATION, 0x12);
    res |= command_send(COMMAND_CONTRAST_SET, 0x80);
    res |= command_send(COMMAND_PRECHARGE_PERIOD_SET, 0xF1);
    res |= command_send(COMMAND_VCOMH_DESELECT_LEVEL_SET, 0x40);
    res |= command_send(COMMAND_ENTIREON_DISABLED);
    res |= command_send(COMMAND_INVERSION_DISABLED);
    for (size_t i = 0; i < (m_gdram_height + 7) / 8; i++) {
        res |= command_send(COMMAND_PAGE_ADDRESS + i);
        res |= command_send(COMMAND_COLUMN_ADDRESS_L | (2 & 0x0F));
        res |= command_send(COMMAND_COLUMN_ADDRESS_H | (2 >> 4));
        for (size_t j = 0; j < m_active_width; j++) {
            res |= data_send(0x00);
        }
    }
    res |= command_send(COMMAND_DISPLAY_ON);
    if (res != 0) {
        return -EIO;
    }

    /* Return success */
    return 0;
}

/**
 *
 */
bool sh1106::detect(void) {
    switch (m_interface) {

        case INTERFACE_I2C_BUFFERED: {  // In I2C: ensure the device ack its address
            if (m_i2c_library != NULL) {
                m_i2c_library->beginTransmission(m_i2c_address);
                if (m_i2c_library->endTransmission() == 0) {
                    return true;
                }
            }
            return false;
        }

        case INTERFACE_SPI_3WIRES:
        case INTERFACE_SPI_4WIRES: {  // In SPI: there is no way to detect the device
            return true;
        }

        default: {
            return false;
        }
    }
}

/**
 *
 * @param[in] ratio
 * @return
 */
int sh1106::brightness_set(const float ratio) {
    if (ratio < 0 || ratio > 1) {
        return -EINVAL;
    }
    return command_send(COMMAND_CONTRAST_SET, (uint8_t)(ratio * 255));
}

/**
 *
 */
int sh1106::inverted_set(const bool inverted) {
    return command_send(inverted ? COMMAND_INVERSION_ENABLED : COMMAND_INVERSION_DISABLED);
}
void sh1106::invertDisplay(bool i) {
    inverted_set(i);
}

/**
 * @return
 */
int sh1106::clear(void) {
    switch (m_interface) {

        case INTERFACE_I2C_LIGHT: {  // For unbuffered interface, clear gdram directly
            for (size_t i = 0; i < (m_gdram_height + 7) / 8; i++) {
                command_send(COMMAND_PAGE_ADDRESS + i);
                command_send(COMMAND_COLUMN_ADDRESS_L | (2 & 0x0F));
                command_send(COMMAND_COLUMN_ADDRESS_H | (2 >> 4));
                for (size_t j = 0; j < m_active_width; j++) {
                    data_send(0x00);
                }
            }
            return 0;
        }

        case INTERFACE_I2C_BUFFERED:
        case INTERFACE_SPI_3WIRES:
        case INTERFACE_SPI_4WIRES: {
            memset(m_buffer, 0, m_active_width * ((m_active_height + 7) / 8));
            return 0;
        }

        default: {
            return -EINVAL;
        }
    }
}

int sh1106::pixel_set(const size_t x, const size_t y, const uint16_t color) {
    int res;

    /* Handle rotation */
    size_t x_panel, y_panel;
    res = m_rotation_handle(x, y, x_panel, y_panel);
    if (res < 0) {
        return -EINVAL;
    }

    /* Modify display data either in local buffer or directly in gdram */
    switch (m_interface) {
        case INTERFACE_I2C_BUFFERED:
        case INTERFACE_SPI_3WIRES:
        case INTERFACE_SPI_4WIRES: {
            if (color) {
                m_buffer[x_panel + (y_panel / 8) * m_active_width] |= (1 << (y_panel % 8));
            } else {
                m_buffer[x_panel + (y_panel / 8) * m_active_width] &= ~(1 << (y_panel % 8));
            }
            break;
        }
        case INTERFACE_I2C_LIGHT: {
            res = 0;
            m_i2c_library->beginTransmission(m_i2c_address);
            m_i2c_library->write((uint8_t)0x80);  // CO = 1, DC = 0
            m_i2c_library->write((uint8_t)(COMMAND_PAGE_ADDRESS + (y_panel / 8)));
            m_i2c_library->write((uint8_t)0x80);  // CO = 1, DC = 0
            m_i2c_library->write((uint8_t)(COMMAND_COLUMN_ADDRESS_L | ((x_panel + 2) & 0x0F)));
            m_i2c_library->write((uint8_t)0x80);  // CO = 1, DC = 0
            m_i2c_library->write((uint8_t)(COMMAND_COLUMN_ADDRESS_H | ((x_panel + 2) >> 4)));
            m_i2c_library->write((uint8_t)0x80);  // CO = 1, DC = 0
            m_i2c_library->write((uint8_t)COMMAND_READWRITEMODIFY_BEGIN);
            m_i2c_library->write((uint8_t)0x40);  // CO = 0, DC = 1
            m_i2c_library->endTransmission(false);
            m_i2c_library->requestFrom(m_i2c_address, 2, true);
            m_i2c_library->read();                 // Dummy read
            uint8_t data = m_i2c_library->read();  // Gdram data
            m_i2c_library->beginTransmission(m_i2c_address);
            m_i2c_library->write((uint8_t)0xC0);  // CO = 1, DC = 1
            if (color) {
                m_i2c_library->write((uint8_t)((1 << (y_panel % 8)) | data));
            } else {
                m_i2c_library->write((uint8_t)((~(1 << (y_panel % 8)) & data)));
            }
            m_i2c_library->write((uint8_t)0x00);  // CO = 0, DC = 0
            m_i2c_library->write((uint8_t)COMMAND_READWRITEMODIFY_END);
            m_i2c_library->endTransmission(true);
            break;
        }
        default: {
            return -EINVAL;
        }
    }

    /* Return success */
    return 0;
}
void sh1106::drawPixel(int16_t x, int16_t y, uint16_t color) {
    pixel_set(x, y, color);
}

/**
 *
 * @return
 */
int sh1106::display(void) {
    int res;
    switch (m_interface) {

        case INTERFACE_I2C_BUFFERED:
        case INTERFACE_SPI_3WIRES:
        case INTERFACE_SPI_4WIRES: {
            for (size_t i = 0, p = 0; i < (m_gdram_height + 7) / 8; i++) {
                command_send(COMMAND_PAGE_ADDRESS + i);
                command_send(COMMAND_COLUMN_ADDRESS_L | (2 & 0x0F));
                command_send(COMMAND_COLUMN_ADDRESS_H | (2 >> 4));
                for (size_t j = 0; j < m_active_width; j++) {
                    data_send(m_buffer[p++]);
                }
            }
            return 0;
        }

        case INTERFACE_I2C_LIGHT: {
            return 0;
        }

        default: {
            return -EINVAL;
        }
    }
}

/**
 *
 * @param[in] command
 */
int sh1106::command_send(const uint8_t command) {
    int res;
    switch (m_interface) {

        case INTERFACE_I2C_BUFFERED:
        case INTERFACE_I2C_LIGHT: {
            m_i2c_library->beginTransmission(m_i2c_address);
            m_i2c_library->write((uint8_t)0x80);
            m_i2c_library->write(command);
            res = m_i2c_library->endTransmission();
            if (res != 0) {
                return -EIO;
            }
            break;
        }

        case INTERFACE_SPI_4WIRES:
        case INTERFACE_SPI_3WIRES: {
            m_spi_library->beginTransaction(m_spi_settings);
            digitalWrite(m_pin_dc, LOW);
            digitalWrite(m_pin_cs, LOW);
            m_spi_library->transfer(command);
            digitalWrite(m_pin_cs, HIGH);
            m_spi_library->endTransaction();
            break;
        }

        default: {
            return -EINVAL;
        }
    }
    return 0;
}

/**
 *
 * @param[in] command
 * @param[in] parameter
 */
int sh1106::command_send(const uint8_t command, const uint8_t parameter) {
    int res;
    switch (m_interface) {

        case INTERFACE_I2C_BUFFERED:
        case INTERFACE_I2C_LIGHT: {
            m_i2c_library->beginTransmission(m_i2c_address);
            m_i2c_library->write((uint8_t)0x80);
            m_i2c_library->write(command);
            m_i2c_library->write((uint8_t)0x80);
            m_i2c_library->write(parameter);
            res = m_i2c_library->endTransmission();
            if (res != 0) {
                return -EIO;
            }
            break;
        }

        case INTERFACE_SPI_4WIRES:
        case INTERFACE_SPI_3WIRES: {
            m_spi_library->beginTransaction(m_spi_settings);
            digitalWrite(m_pin_dc, LOW);
            digitalWrite(m_pin_cs, LOW);
            m_spi_library->transfer(command);
            m_spi_library->transfer(parameter);
            digitalWrite(m_pin_cs, HIGH);
            m_spi_library->endTransaction();
            break;
        }

        default: {
            return -EINVAL;
        }
    }
    return 0;
}

/**
 *
 * @param[in] data
 */
int sh1106::data_send(const uint8_t data) {
    int res;
    switch (m_interface) {

        case INTERFACE_I2C_BUFFERED:
        case INTERFACE_I2C_LIGHT: {
            m_i2c_library->beginTransmission(m_i2c_address);
            m_i2c_library->write((uint8_t)0x40);
            m_i2c_library->write(data);
            res = m_i2c_library->endTransmission();
            if (res != 0) {
                return -EIO;
            }
            break;
        }

        case INTERFACE_SPI_4WIRES:
        case INTERFACE_SPI_3WIRES: {
            m_spi_library->beginTransaction(m_spi_settings);
            digitalWrite(m_pin_dc, HIGH);
            digitalWrite(m_pin_cs, LOW);
            m_spi_library->transfer(data);
            digitalWrite(m_pin_cs, HIGH);
            m_spi_library->endTransaction();
            break;
        }

        default: {
            return -EINVAL;
        }
    }
    return 0;
}

/**
 *
 * @param[in] data
 * @param[in] length
 */
int sh1106::data_send(uint8_t* const data, const size_t length) {
    int res;
    switch (m_interface) {

        case INTERFACE_I2C_BUFFERED:
        case INTERFACE_I2C_LIGHT: {
            m_i2c_library->beginTransmission(m_i2c_address);
            m_i2c_library->write((uint8_t)0x40);
            m_i2c_library->write(data, length);
            res = m_i2c_library->endTransmission();
            if (res != 0) {
                return -EIO;
            }
            break;
        }

        case INTERFACE_SPI_4WIRES:
        case INTERFACE_SPI_3WIRES: {
            m_spi_library->beginTransaction(m_spi_settings);
            digitalWrite(m_pin_dc, HIGH);
            digitalWrite(m_pin_cs, LOW);
            for (size_t i = 0; i < length; i++) {
                m_spi_library->transfer(data[i]);
            }
            digitalWrite(m_pin_cs, HIGH);
            m_spi_library->endTransaction();
            break;
        }

        default: {
            return -EINVAL;
        }
    }
    return 0;
}

int sh1106::m_rotation_handle(const size_t x, const size_t y, size_t& x_panel, size_t& y_panel) const {
    if (x >= m_active_width || y >= m_active_height) {
        return -EINVAL;
    }
    x_panel = x;
    y_panel = y;
    switch (getRotation()) {
        case 0:
            break;
        case 1:
            x_panel = m_active_width - y - 1;
            y_panel = x;
            break;
        case 2:
            x_panel = m_active_width - x - 1;
            y_panel = m_active_height - y - 1;
            break;
        case 3:
            x_panel = y;
            y_panel = m_active_height - x - 1;
            break;
    }
    return 0;
}
