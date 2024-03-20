package com.relsa.desurep.controllers;

import com.relsa.desurep.services.FileStorageService;
import com.relsa.desurep.services.UserService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Controller;
import org.springframework.ui.Model;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.servlet.mvc.support.RedirectAttributes;

import java.io.Console;
import java.util.Objects;

@Controller
public class SignupController {
    @Autowired
    private FileStorageService fileStorageService;

    @Autowired
    UserService userService;

    @GetMapping("/signup")
    public String signup(Model model,
                         @RequestParam(required=false) boolean error,
                         RedirectAttributes redirectAttrs) {
        if (error)
            redirectAttrs.addAttribute("error", true);
        return "signup";
    }

    @PostMapping("/signup")
    public String signupRequest(@RequestParam(value = "username", required=false) String username,
                                @RequestParam(value = "email", required=false) String email,
                                @RequestParam(value = "password", required=false) String password,
                                @RequestParam(value = "passwordSec", required=false) String passwordSec) {
        if(Objects.equals(password, passwordSec)) {
            if (userService.findUserByEmail(email) != null || userService.findUserByUsername(username) != null) {
                return "redirect:/signup/?error=true";
            }
            userService.saveUser(username, email, password);
            return "redirect:/";
        }
        else
            return "redirect:/signup/?error=true";
    }
}
