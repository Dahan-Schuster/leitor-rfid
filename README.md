# Leitor RFID Wireless
Leitor RFID wireless com envio de dados via Wi-Fi.

### Hardware
O dispositivo é formado por duas placas eletrônicas: O microcontrolador com módulo Wi-Fi e Bluetooth **ESP32** e o leitor RFID **MFRC-522**

### Sistema
Este leitor foi pensado para funcionar em conjunto com o sistema web Bike IFS, feito com HTML, CSS, JavaScript, PHP e PostgreSQL.
O sistema funciona independente do leitor, sendo este apenas uma alternativa à inserção manual de dados. Bike IFS é um sistema para validação e controle de acesso de bicicletários, e os dados referentes às entradas e saídas devem ser inseridos via teclado. O leitor RFID detecta *tags mifare* HF de frequência 13,56 MHz previamente cadastradas e presas em bicicletas e alimenta o sistema com os dados da mesma, automatizando o registro.
