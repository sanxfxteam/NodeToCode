# Compact Python pseudo-code for BP_ThirdPersonCharacter
# Generated from Tests\code.json N2C format

class BP_ThirdPersonCharacter(Character):
    
    def __init__(self):
        self.IA_Jump = InputAction("IA_Jump")
        self.IA_Move = InputAction("IA_Move") 
        self.IA_Look = InputAction("IA_Look")
        self.IMC_Default = InputMappingContext("/Game/ThirdPerson/Input/IMC_Default")

    # Event: Controller changed (N17 → N14 → N15)
    def on_controller_changed(self, old_controller, new_controller):
        if player_ctrl := cast(new_controller, PlayerController):
            input_sys = player_ctrl.get_subsystem(EnhancedInputLocalPlayerSubsystem)
            input_sys.add_mapping_context(self.IMC_Default, priority=0)

    # Input: Jump actions (N11 → N1, N7)
    def on_jump_started(self): 
        self.jump()
    
    def on_jump_completed(self): 
        self.stop_jumping()

    # Input: Movement (N12 → N5 → N4, with N6,N8,N9,N10 for vectors)
    def on_move(self, input_2d):
        rotation = self.get_control_rotation()
        forward = get_forward_vector(rotation)
        right = get_right_vector(rotation)
        self.add_movement_input(forward, input_2d.y)  # Forward/back
        self.add_movement_input(right, input_2d.x)    # Left/right

    # Input: Look (N13 → N3 → N2)
    def on_look(self, input_2d):
        self.add_controller_yaw_input(input_2d.x)    # Horizontal mouse
        self.add_controller_pitch_input(input_2d.y)  # Vertical mouse

    # Setup input bindings (conceptual)
    def setup_input(self):
        bind_action(self.IA_Jump, "started", self.on_jump_started)
        bind_action(self.IA_Jump, "completed", self.on_jump_completed)  
        bind_action(self.IA_Move, "triggered", self.on_move)
        bind_action(self.IA_Look, "triggered", self.on_look)

# Data flow summary:
# control_rotation.yaw → forward_vector → movement_forward
# control_rotation → right_vector → movement_right  
# move_input.x → right_movement_scale
# move_input.y → forward_movement_scale
# look_input.x → yaw_input
# look_input.y → pitch_input