workspace "Followy.Booking Platform" "Архитектура платформы бронирования отелей" {
  !identifiers hierarchical

  model {
    client = person "Клиент" "Ищет отели, совершает и отменяет бронирования."
    hotel_admin = person "Администратор отеля" "Управляет номерным фондом своего отеля."


    smtp_ext = softwareSystem "External Mail Provider" "Внешний почтовый сервис (по типу mail.ru)." "External"


    platform = softwareSystem "Followy.Booking System" "Внутренняя система бронирования" {

      client_app = container "Multiplatform App" "Кроссплатформенное приложение (минимально - Web, с перспективой расширения до Android + iOS)" "Kotlin Multiplatform"
      
      edge_proxy = container "API Gateway" "Точка входа. Роутинг, валидация JWT." "Nginx"

      auth_service = container "Auth Service" "Внутренний сервис аутентификации. Регистрация, логин, выдача JWT." "C++ userver"
      auth_db = container "Auth DB" "Хранение хешей паролей и учетных данных." "PostgreSQL" "Database"

      inventory_service = container "Inventory Service" "Каталог отелей и управление наличием свободных номеров." "C++ userver"
      inventory_db = container "Inventory DB" "Справочник отелей, номеров и их доступности." "PostgreSQL" "Database"

      booking_service = container "Booking Service" "Управление транзакциями бронирований." "C++ userver"
      booking_db = container "Booking DB" "Хранение статусов и истории броней." "PostgreSQL" "Database"

      message_broker = container "Message Broker" "Очередь событий платформы (только для писем)." "Apache Kafka" "Queue"
      
      mailer_service = container "Mailer Service" "Слушает очередь и отправляет уведомления клиентам." "C++ worker"


      edge_proxy -> auth_service "Проксирует /api/auth" "REST"
      edge_proxy -> inventory_service "Проксирует /api/hotels" "REST"
      edge_proxy -> booking_service "Проксирует /api/bookings" "REST"

      auth_service -> auth_db "Манипуляции над пользовательскими данными" "TCP/SQL"
      inventory_service -> inventory_db "Манипуляции с каталожными данными" "TCP/SQL"
      booking_service -> booking_db "Манипуляции с данными о бронированиях" "TCP/SQL"

      booking_service -> inventory_service "Резервирование номеров" "HTTPS/REST"

      booking_service -> message_broker "Публикует события" "TCP"
      mailer_service -> message_broker "Читает топики уведомлений" "TCP"
      mailer_service -> smtp_ext "Отправляет сгенерированные Email" "SMTP"
    }


    client -> platform.client_app "Использует приложение" "HTTPS"
    hotel_admin -> platform.client_app "Использует приложение" "HTTPS"

    platform.client_app -> platform.edge_proxy "API Requests" "HTTPS/REST"
  }

  views {
    themes default

    systemContext platform "Context_Diagram" "System Context" {
      include *
      autolayout tb
    }

    container platform "Container_Diagram" "Container view" {
      include *
      autolayout tb
    }

    dynamic platform "Scenario_Auth" "Сценарий: Внутренняя аутентификация клиента" {
      autolayout lr
      client -> platform.client_app "Вводит логин и пароль, жмёт на кнопку 'Войти'"
      platform.client_app -> platform.edge_proxy "POST /api/auth/login"
      platform.edge_proxy -> platform.auth_service "Проксирует запрос на авторизацию"
      platform.auth_service -> platform.auth_db "SELECT: сверяет хеш пароля"
      platform.auth_service -> platform.auth_db "UPDATE: обновляет время последнего логина"
      platform.auth_service -> platform.edge_proxy "200 OK + JWT-токен"
      platform.edge_proxy -> platform.client_app "Возвращает токен клиенту"
    }

    dynamic platform "Scenario_CreateBooking" "Сценарий: Клиент оформляет бронь (Успешно)" {
      autolayout lr
      client -> platform.client_app "Нажимает 'Забронировать'"
      platform.client_app -> platform.edge_proxy "POST /api/bookings (с JWT)"
      platform.edge_proxy -> platform.booking_service "Передает запрос (с ID клиента в заголовке X-User-Id)"
      platform.booking_service -> platform.inventory_service "POST /api/internal/inventory/{id}/reserve"
      platform.inventory_service -> platform.inventory_db "UPDATE inventory SET available = available - 1"
      platform.inventory_service -> platform.booking_service "200 OK (Номер Успешно зарезервирован)"
      platform.booking_service -> platform.booking_db "INSERT: сохраняет бронь со статусом CONFIRMED"
      platform.booking_service -> platform.message_broker "Event: BookingCreated (для письма)"
      platform.mailer_service -> platform.message_broker "Читает событие BookingCreated"
      platform.mailer_service -> smtp_ext "SMTP: Отправляет письмо с подтверждением"
      platform.booking_service -> platform.edge_proxy "201 Created (ID брони)"
      platform.edge_proxy -> platform.client_app "Бронь оформлена"
    }

    styles {
      element "Person" {
        shape person
        background #0B4A7B
        color #ffffff
      }
      element "Software System" {
        background #1061A0
        color #ffffff
      }
      element "Container" {
        background #2196F3
        color #ffffff
      }
      element "Database" {
        shape cylinder
        background #F9A825
        color #000000
      }
      element "Queue" {
        shape pipe
        background #E65100
        color #ffffff
      }
      element "External" {
        background #757575
        color #ffffff 
      }
    }
  }
}
