use iced::widget::{button, column, container, row, text};
use iced::{executor, time, Alignment, Application, Background, Color, Command, Element, Length, Settings, Subscription, Theme};
use std::time::{Duration, Instant};

pub fn main() -> iced::Result {
    Pomodoro::run(Settings::default())
}

struct Pomodoro {
    duration: u32,       // Seconds remaining
    initial_duration: u32,
    state: TimerState,
    last_tick: Option<Instant>,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum TimerState {
    Idle,
    Running,
    Paused,
}

#[derive(Debug, Clone, Copy)]
enum Message {
    ToggleTimer,
    ResetTimer,
    Tick(Instant),
}

impl Application for Pomodoro {
    type Executor = executor::Default;
    type Message = Message;
    type Theme = Theme;
    type Flags = ();

    fn new(_flags: ()) -> (Self, Command<Message>) {
        (
            Pomodoro {
                duration: 25 * 60,
                initial_duration: 25 * 60,
                state: TimerState::Idle,
                last_tick: None,
            },
            Command::none(),
        )
    }

    fn title(&self) -> String {
        String::from("Pomodoro Timer")
    }

    fn update(&mut self, message: Message) -> Command<Message> {
        match message {
            Message::ToggleTimer => {
                match self.state {
                    TimerState::Idle => {
                        self.state = TimerState::Running;
                        self.last_tick = Some(Instant::now());
                    }
                    TimerState::Running => {
                        self.state = TimerState::Paused;
                    }
                    TimerState::Paused => {
                        self.state = TimerState::Running;
                        self.last_tick = Some(Instant::now());
                    }
                }
            }
            Message::ResetTimer => {
                self.state = TimerState::Idle;
                self.duration = self.initial_duration;
            }
            Message::Tick(_now) => {
                if self.state == TimerState::Running {
                    if self.duration > 0 {
                        self.duration -= 1;
                    } else {
                        // Timer finished
                        self.state = TimerState::Idle;
                        self.duration = self.initial_duration;
                    }
                }
            }
        }
        Command::none()
    }

    fn view(&self) -> Element<Message> {
        let minutes = self.duration / 60;
        let seconds = self.duration % 60;

        let time_text = text(format!("{:02}:{:02}", minutes, seconds)).size(50);

        let toggle_text = match self.state {
            TimerState::Idle => "Start",
            TimerState::Running => "Pause",
            TimerState::Paused => "Resume",
        };

        let start_button = button(text(toggle_text))
            .padding(10)
            .on_press(Message::ToggleTimer);

        let reset_button = button(text("Reset"))
            .padding(10)
            .on_press(Message::ResetTimer);

        let content = column![
            time_text,
            row![start_button, reset_button].spacing(20)
        ]
        .spacing(20)
        .align_items(Alignment::Center);

        container(content)
            .width(Length::Fill)
            .height(Length::Fill)
            .center_x()
            .center_y()
            .style(|_: &_| container::Appearance {
                background: Some(Background::Color(Color::BLACK)),
                text_color: Some(Color::WHITE),
                ..Default::default()
            })
            .into()
    }

    fn subscription(&self) -> Subscription<Message> {
        match self.state {
            TimerState::Running => {
                time::every(Duration::from_secs(1)).map(Message::Tick)
            }
            _ => Subscription::none(),
        }
    }
}
