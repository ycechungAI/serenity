@GUI::Frame {
    fill_with_background_color: true

    layout: @GUI::VerticalBoxLayout {
        margins: [10]
        spacing: 5
    }

    @GUI::GroupBox {
        title: "Server Settings"
        fixed_height: 170

        layout: @GUI::VerticalBoxLayout {
            margins: [16, 8, 8]
            spacing: 2
        }

        @GUI::Widget {
            layout: @GUI::HorizontalBoxLayout {
                spacing: 16
            }

            @GUI::Label {
                fixed_width: 32
                fixed_height: 32
                name: "server_settings_image_label"
            }

            @GUI::Label {
                text: "These settings specify the mail server from which you would like to fetch your mail."
                text_alignment: "CenterLeft"
            }
        }

        @GUI::Widget {
            layout: @GUI::HorizontalBoxLayout {
                spacing: 16
            }

            @GUI::Widget {
                fixed_width: 32
            }

            @GUI::Label {
                text: "Server Address:"
                fixed_width: 80
                name: "server_label"
                text_alignment: "CenterLeft"
            }

            @GUI::TextBox {
                name: "server_input"
            }
        }
        @GUI::Widget {
            layout: @GUI::HorizontalBoxLayout {
                spacing: 16
            }

            @GUI::Widget {
                fixed_width: 32
            }

            @GUI::Label {
                text: "Server Port:"
                fixed_width: 80
                name: "port_label"
                text_alignment: "CenterLeft"
            }

            @GUI::ComboBox {
                name: "port_input"
            }
        }
        @GUI::Widget {
            layout: @GUI::HorizontalBoxLayout {
                spacing: 16
            }

            @GUI::Widget {
                fixed_width: 32
            }

            @GUI::Label {
                text: "Use TLS:"
                fixed_width: 80
                text_alignment: "CenterLeft"
                name: "tls_label"
            }

            @GUI::CheckBox {
                name: "tls_input"
                fixed_width: 14

            }
        }
    }

    @GUI::GroupBox {
        title: "User Settings"
        fixed_height: 110

        layout: @GUI::VerticalBoxLayout {
            margins: [16, 8, 8]
            spacing: 2
        }

        @GUI::Widget {
            layout: @GUI::HorizontalBoxLayout {
                spacing: 16
            }

            @GUI::Label {
                fixed_width: 32
                fixed_height: 32
                name: "user_settings_image_label"
            }

            @GUI::Label {
                text: "These settings specify the credentials which will be used to send and receive mail, and how you identify yourself to recipients."
                text_alignment: "CenterLeft"
            }
        }

        @GUI::Widget {
            layout: @GUI::HorizontalBoxLayout {
                spacing: 16
            }

            @GUI::Widget {
                fixed_width: 32
            }

            @GUI::Label {
                autosize: true
                text: "Email Address:"
                fixed_width: 80
                text_alignment: "CenterLeft"
            }

            @GUI::TextBox {
                name: "email_input"
            }
        }
    }
}
